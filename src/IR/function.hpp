#ifndef QAT_IR_FUNCTION_HPP
#define QAT_IR_FUNCTION_HPP

#include "../utils/file_range.hpp"
#include "../utils/helpers.hpp"
#include "../utils/identifier.hpp"
#include "../utils/visibility.hpp"
#include "./argument.hpp"
#include "./value.hpp"
#include "template_variant.hpp"
#include "types/qat_type.hpp"
#include "uniq.hpp"
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
class TemplatedType;
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

class LocalValue : public Value, public Uniq {
  String           name;
  Maybe<FileRange> fileRange;

public:
  LocalValue(String name, IR::QatType* type, bool isVariable, Function* fun, Maybe<FileRange> fileRange = None);
  ~LocalValue() = default;

  useit String getName() const;
  useit llvm::AllocaInst* getAlloca() const;
};

class Block : public Uniq {
private:
  String                                name;
  llvm::BasicBlock*                     bb;
  Vec<LocalValue*>                      values;
  Block*                                parent;
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
  useit LocalValue* newValue(const String& name, IR::QatType* type, bool isVar, Maybe<FileRange> fileRange = None);
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
};

// Function represents a normal function in the language
class Function : public Value, public Uniq {
  friend class Block;

protected:
  Identifier            name;
  bool                  isReturnValueVariable;
  QatModule*            mod;
  Vec<Argument>         arguments;
  utils::VisibilityInfo visibility_info;
  FileRange             fileRange;
  bool                  is_async;
  bool                  hasVariadicArguments;
  Vec<Block*>           blocks;

  mutable usize activeBlock   = 0;
  mutable usize copiedCounter = 0;
  mutable usize movedCounter  = 0;
  mutable u64   calls;
  mutable u64   refers;

  Maybe<llvm::Function*>   asyncFn;
  Maybe<llvm::StructType*> asyncArgTy;

  Function(QatModule* mod, Identifier _name, QatType* returnType, bool _isReturnValueVariable, bool _is_async,
           Vec<Argument> _args, bool has_variadic_arguments, FileRange fileRange,
           const utils::VisibilityInfo& _visibility_info, llvm::LLVMContext& ctx, bool isMemberFn = false,
           llvm::GlobalValue::LinkageTypes _linkage         = llvm::GlobalValue::LinkageTypes::WeakAnyLinkage,
           bool                            ignoreParentName = false);

public:
  static Function*   Create(QatModule* mod, Identifier name, QatType* return_type, bool isReturnValueVariable,
                            bool is_async, Vec<Argument> args, bool has_variadic_args, FileRange fileRange,
                            const utils::VisibilityInfo& visibilityInfo, llvm::LLVMContext& ctx,
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
  useit bool               isAccessible(const utils::RequesterInfo& req_info) const;
  useit utils::VisibilityInfo getVisibility() const;
  useit llvm::Function* getLLVMFunction();
  void                  setActiveBlock(usize index) const;
  useit Block*          getBlock() const;
  useit Block*          getFirstBlock() const;
  useit usize           getBlockCount() const;
  useit usize&          getCopiedCounter();
  useit usize&          getMovedCounter();

  ~Function() override;
};

class TemplateFunction : public Uniq {
private:
  Identifier               name;
  Vec<ast::TemplatedType*> templates;
  ast::FunctionDefinition* functionDefinition;
  QatModule*               parent;
  utils::VisibilityInfo    visibInfo;

  mutable Vec<TemplateVariant<Function>> variants;

public:
  TemplateFunction(Identifier name, Vec<ast::TemplatedType*> templates, ast::FunctionDefinition* functionDef,
                   QatModule* parent, const utils::VisibilityInfo& _visibInfo);

  ~TemplateFunction() = default;

  useit Identifier getName() const;
  useit usize      getTypeCount() const;
  useit usize      getVariantCount() const;
  useit QatModule* getModule() const;
  useit utils::VisibilityInfo getVisibility() const;
  useit Function*             fillTemplates(Vec<IR::QatType*> _types, IR::Context* ctx, const FileRange& fileRange);
};

void functionReturnHandler(IR::Context* ctx, IR::Function* fun, const FileRange& fileRange);
void destructorCaller(IR::Context* ctx, IR::Function* fun);
void memberFunctionHandler(IR::Context* ctx, IR::Function* fun);
void destroyLocalsFrom(IR::Context* ctx, IR::Block* block);

} // namespace qat::IR

#endif