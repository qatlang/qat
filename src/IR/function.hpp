#ifndef QAT_IR_FUNCTION_HPP
#define QAT_IR_FUNCTION_HPP

#include "../utils/file_range.hpp"
#include "../utils/helpers.hpp"
#include "../utils/identifier.hpp"
#include "../utils/visibility.hpp"
#include "./argument.hpp"
#include "./entity_overview.hpp"
#include "./generic_variant.hpp"
#include "./generics.hpp"
#include "./types/qat_type.hpp"
#include "./uniq.hpp"
#include "./value.hpp"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include <iostream>
#include <string>
#include <vector>

namespace qat::ast {
class FunctionDefinition;
class GenericAbstractType;
} // namespace qat::ast

namespace qat::IR {

class QatModule;
class FunctionCall;
class Context;
class Function;

enum class ExternFnType {
  C,
  CPP,
};

class LocalValue final : public Value, public Uniq, public EntityOverview {
  String    name;
  FileRange fileRange;

public:
  LocalValue(String name, IR::QatType* type, bool isVariable, Function* fun, FileRange fileRange);
  ~LocalValue() final = default;

  useit String getName() const;
  useit llvm::AllocaInst* getAlloca() const;
  useit FileRange         getFileRange() const;
  useit IR::Value* toNewIRValue() const;
};

class Block : public Uniq {
private:
  String                                name;
  llvm::BasicBlock*                     bb;
  Vec<LocalValue*>                      values;
  Block*                                parent = nullptr;
  Vec<Block*>                           children;
  Function*                             fn;
  usize                                 index;
  Maybe<usize>                          active;
  mutable Vec<String>                   movedValues;
  mutable bool                          isGhost = false;
  mutable bool                          hasGive = false;
  mutable Vec<Pair<String, IR::Value*>> aliases;

  Block* prevBlock = nullptr;
  Block* nextBlock = nullptr;

public:
  Block(Function* _fn, Block* _parent);

  ~Block() = default;

  useit String getName() const;
  useit llvm::BasicBlock* getBB() const;

  useit bool hasPrevBlock() const;
  Block*     getPrevBlock() const;
  useit bool hasNextBlock() const;
  Block*     getNextBlock() const;
  void       linkPrevBlock(Block* block);

  useit bool        hasParent() const;
  useit Block*      getParent() const;
  useit Function*   getFn() const;
  useit bool        hasValue(const String& name) const;
  useit LocalValue* getValue(const String& name) const;
  useit LocalValue* newValue(const String& name, IR::QatType* type, bool isVar, FileRange fileRange);
  useit bool        isMoved(const String& locID) const;
  useit bool        hasGiveInAllControlPaths() const;
  useit Block*      getActive();
  useit Vec<LocalValue*>& getLocals();
  void                    setGhost(bool value) const;
  void                    setHasGive() const;
  void                    addMovedValue(String locID) const;
  void                    setActive(llvm::IRBuilder<>& builder);
  void                    collectAllLocalValuesSoFar(Vec<LocalValue*>& vals) const;
  void                    collectLocalsFrom(Vec<LocalValue*>& vals) const;
  void                    destroyLocals(IR::Context* ctx);
  void                    outputLocalOverview(Vec<JsonValue>& jsonVals);
};

// Function represents a normal function in the language
class Function : public Value, public Uniq, public EntityOverview {
  friend class Block;

protected:
  Identifier             name;
  Vec<GenericParameter*> generics;
  QatModule*             mod;
  Vec<Argument>          arguments;
  VisibilityInfo         visibility_info;
  FileRange              fileRange;
  bool                   is_async;
  bool                   hasVariadicArguments;
  Vec<Block*>            blocks;
  IR::LocalValue*        strComparisonIndex = nullptr;

  mutable usize activeBlock   = 0;
  mutable usize copiedCounter = 0;
  mutable usize movedCounter  = 0;
  mutable u64   calls;
  mutable u64   refers;

  Maybe<llvm::Function*>   asyncFn;
  Maybe<llvm::StructType*> asyncArgTy;

  Function(QatModule* mod, Identifier _name, Vec<GenericParameter*> _generics, QatType* returnType, bool _is_async,
           Vec<Argument> _args, bool has_variadic_arguments, FileRange fileRange,
           const VisibilityInfo& _visibility_info, IR::Context* ctx, bool isMemberFn = false,
           llvm::GlobalValue::LinkageTypes _linkage         = llvm::GlobalValue::LinkageTypes::WeakAnyLinkage,
           bool                            ignoreParentName = false);

public:
  static Function*   Create(QatModule* mod, Identifier name, Vec<GenericParameter*> _generics, QatType* return_type,
                            bool is_async, Vec<Argument> args, bool has_variadic_args, FileRange fileRange,
                            const VisibilityInfo& visibilityInfo, IR::Context* ctx,
                            llvm::GlobalValue::LinkageTypes linkage = llvm::GlobalValue::LinkageTypes::WeakAnyLinkage,
                            bool                            ignoreParentName = false);
  useit Value*       call(IR::Context* ctx, const Vec<llvm::Value*>& args, QatModule* mod) override;
  useit virtual bool isMemberFunction() const;
  useit bool         hasVariadicArgs() const;
  useit bool         isAsyncFunction() const;
  useit llvm::Function* getAsyncSubFunction() const;
  useit llvm::StructType*  getAsyncArgType() const;
  useit Identifier         argumentNameAt(u32 index) const;
  useit virtual Identifier getName() const;
  useit virtual String     getFullName() const;
  useit bool               hasReturnArgument() const;
  useit bool               isReturnTypeReference() const;
  useit bool               isReturnTypePointer() const;
  useit bool               isAccessible(const AccessInfo& req_info) const;
  useit VisibilityInfo     getVisibility() const;
  useit llvm::Function* getLLVMFunction();
  void                  setActiveBlock(usize index) const;
  useit Block*          getBlock() const;
  useit Block*          getFirstBlock() const;
  useit usize           getBlockCount() const;
  useit usize&          getCopiedCounter();
  useit usize&          getMovedCounter();
  useit IR::LocalValue*   getFunctionCommonIndex();
  useit bool              hasGenericParameter(const String& name) const;
  useit GenericParameter* getGenericParameter(const String& name) const;

  void updateOverview() override;

  ~Function() override;
};

class GenericFunction : public Uniq, public EntityOverview {
private:
  Identifier                     name;
  Vec<ast::GenericAbstractType*> generics;
  ast::FunctionDefinition*       functionDefinition;
  QatModule*                     parent;
  VisibilityInfo                 visibInfo;

  mutable Vec<GenericVariant<Function>> variants;

public:
  GenericFunction(Identifier name, Vec<ast::GenericAbstractType*> _generics, ast::FunctionDefinition* functionDef,
                  QatModule* parent, const VisibilityInfo& _visibInfo);

  ~GenericFunction() = default;

  useit Identifier getName() const;
  useit usize      getTypeCount() const;
  useit usize      getVariantCount() const;
  useit QatModule* getModule() const;
  useit ast::GenericAbstractType* getGenericAt(usize index) const;
  useit VisibilityInfo            getVisibility() const;
  useit Function* fillGenerics(Vec<IR::GenericToFill*> _types, IR::Context* ctx, const FileRange& fileRange);
  useit bool      allTypesHaveDefaults() const;
};

void functionReturnHandler(IR::Context* ctx, IR::Function* fun, const FileRange& fileRange);
void destructorCaller(IR::Context* ctx, IR::Function* fun);
void memberFunctionHandler(IR::Context* ctx, IR::Function* fun);
void destroyLocalsFrom(IR::Context* ctx, IR::Block* block);

} // namespace qat::IR

#endif