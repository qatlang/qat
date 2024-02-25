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
#include "link_names.hpp"
#include "meta_info.hpp"
#include "types/function.hpp"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"

#define DEFAULT_FUNCTION_LINKAGE llvm::GlobalValue::LinkageTypes::ExternalLinkage

namespace qat::ast {
class FunctionPrototype;
class GenericAbstractType;
class ConstructorDefinition;
class ConvertorDefinition;
class PrerunExpression;
} // namespace qat::ast

namespace qat::IR {

class QatModule;
class FunctionCall;
class Context;
class MemberFunction;

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
  friend MemberFunction;

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
  Maybe<FileRange> fileRange;

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
  useit Maybe<FileRange> getFileRange() const;

  void setFileRange(FileRange _fileRange);
  void setGhost(bool value) const;
  void setHasGive() const;
  void addMovedValue(String locID) const;
  void setActive(llvm::IRBuilder<>& builder);
  void collectAllLocalValuesSoFar(Vec<LocalValue*>& vals) const;
  void collectLocalsFrom(Vec<LocalValue*>& vals) const;
  void destroyLocals(IR::Context* ctx);
  void outputLocalOverview(Vec<JsonValue>& jsonVals);
};

// Function represents a normal function in the language
class Function : public Value, public Uniq, public EntityOverview {
  friend class Block;

protected:
  Identifier             name;
  LinkNames              namingInfo;
  String                 linkingName;
  Vec<GenericParameter*> generics;
  QatModule*             mod;
  Vec<Argument>          arguments;
  VisibilityInfo         visibility_info;
  Maybe<FileRange>       fileRange;
  bool                   hasVariadicArguments;
  Vec<Block*>            blocks;
  IR::LocalValue*        strComparisonIndex = nullptr;
  Maybe<MetaInfo>        metaInfo;
  IR::Context*           ctx;

  mutable u64   localNameCounter = 0;
  mutable usize activeBlock      = 0;

  mutable usize copiedCounter = 0;
  mutable usize movedCounter  = 0;
  mutable u64   calls;
  mutable u64   refers;

  Maybe<llvm::Function*>   asyncFn;
  Maybe<llvm::StructType*> asyncArgTy;

  Function(QatModule* mod, Identifier _name, Maybe<LinkNames> _namingInfo, Vec<GenericParameter*> _generics,
           ReturnType* returnType, Vec<Argument> _args, bool has_variadic_arguments, Maybe<FileRange> fileRange,
           const VisibilityInfo& _visibility_info, IR::Context* ctx, bool isMemberFn = false,
           Maybe<llvm::GlobalValue::LinkageTypes> _linkage = None, Maybe<MetaInfo> _metaInfo = None);

public:
  static Function* Create(QatModule* mod, Identifier name, Maybe<LinkNames> _namingInfo,
                          Vec<GenericParameter*> _generics, ReturnType* return_type, Vec<Argument> args,
                          bool has_variadic_args, Maybe<FileRange> fileRange, const VisibilityInfo& visibilityInfo,
                          IR::Context* ctx, Maybe<llvm::GlobalValue::LinkageTypes> linkage = None,
                          Maybe<MetaInfo> metaInfo = None);
  useit Value* call(IR::Context* ctx, const Vec<llvm::Value*>& args, Maybe<String> localID, QatModule* mod) override;
  useit virtual bool       isMemberFunction() const;
  useit bool               hasVariadicArgs() const;
  useit Identifier         argumentNameAt(u32 index) const;
  useit virtual Identifier getName() const;
  useit virtual String     getFullName() const;
  useit bool               isAccessible(const AccessInfo& req_info) const;
  useit VisibilityInfo     getVisibility() const;
  useit IR::QatModule* getParentModule() const;
  useit llvm::Function* getLLVMFunction();
  void                  setActiveBlock(usize index) const;
  useit Block*          getBlock() const;
  useit Block*          getFirstBlock() const;
  useit usize           getBlockCount() const;
  useit usize&          getCopiedCounter();
  useit usize&          getMovedCounter();
  useit IR::LocalValue*   getFunctionCommonIndex();
  useit bool              isGeneric() const;
  useit bool              hasGenericParameter(const String& name) const;
  useit GenericParameter* getGenericParameter(const String& name) const;
  useit bool              hasDefinitionRange() const;
  useit FileRange         getDefinitionRange() const;
  useit String            getRandomAllocaName() const;

  void updateOverview() override;

  ~Function() override;
};

class GenericFunction : public Uniq, public EntityOverview {
private:
  Identifier                     name;
  Vec<ast::GenericAbstractType*> generics;
  ast::FunctionPrototype*        functionDefinition;
  Maybe<ast::PrerunExpression*>  constraint;
  QatModule*                     parent;
  VisibilityInfo                 visibInfo;

  mutable Vec<GenericVariant<Function>> variants;

public:
  GenericFunction(Identifier name, Vec<ast::GenericAbstractType*> _generics, Maybe<ast::PrerunExpression*> constraint,
                  ast::FunctionPrototype* functionDef, QatModule* parent, const VisibilityInfo& _visibInfo);

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