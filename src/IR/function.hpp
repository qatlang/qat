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

namespace qat::ir {

class Mod;
class FunctionCall;
class Ctx;
class Method;

enum class ExternFnType {
  C,
  CPP,
};

class LocalValue final : public Value, public Uniq, public EntityOverview {
  String    name;
  FileRange fileRange;

public:
  LocalValue(String name, ir::Type* type, bool is_variable, Function* fun, FileRange fileRange);

  useit static inline LocalValue* get(String name, ir::Type* type, bool isVar, Function* fn, FileRange fileRange) {
    return new LocalValue(name, type, isVar, fn, fileRange);
  }

  ~LocalValue() final = default;

  useit String get_name() const;
  useit llvm::AllocaInst* getAlloca() const;
  useit FileRange         get_file_range() const;
  useit ir::Value* to_new_ir_value() const;
};

class Block : public Uniq {
  friend Method;

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
  mutable bool                          hasTodo = false;
  mutable Vec<Pair<String, ir::Value*>> aliases;

  Block* prevBlock = nullptr;
  Block* nextBlock = nullptr;

public:
  Maybe<FileRange> fileRange;

  Block(Function* _fn, Block* _parent);

  ~Block() = default;

  useit String get_name() const;
  useit llvm::BasicBlock* get_bb() const;

  useit bool has_previous_block() const;
  Block*     get_previous_block() const;
  useit bool has_next_block() const;
  Block*     get_next_block() const;
  void       link_previous_block(Block* block);

  useit bool        has_parent() const;
  useit Block*      getParent() const;
  useit Function*   getFn() const;
  useit bool        hasValue(const String& name) const;
  useit LocalValue* getValue(const String& name) const;
  useit LocalValue* new_value(const String& name, ir::Type* type, bool isVar, FileRange fileRange);
  useit bool        is_moved(const String& locID) const;
  useit bool        has_give_in_all_control_paths() const;
  useit bool        has_todo() const { return hasTodo; }
  useit Block*      getActive();
  useit Vec<LocalValue*>& get_locals();
  useit Maybe<FileRange> get_file_range() const;

  void setFileRange(FileRange _fileRange);
  void set_has_give() const;
  void set_has_todo() const { hasTodo = true; }
  void addMovedValue(String locID) const;
  void set_active(llvm::IRBuilder<>& builder);
  void collect_all_local_values_so_far(Vec<LocalValue*>& vals) const;
  void collect_locals_from(Vec<LocalValue*>& vals) const;
  void destroyLocals(ast::EmitCtx* ctx);
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
  Mod*                   mod;
  Vec<Argument>          arguments;
  VisibilityInfo         visibility_info;
  Maybe<FileRange>       fileRange;
  bool                   hasVariadicArguments;
  Vec<Block*>            blocks;
  ir::LocalValue*        strComparisonIndex = nullptr;
  Maybe<MetaInfo>        metaInfo;
  ir::Ctx*               ctx;

  mutable u64   localNameCounter = 0;
  mutable usize activeBlock      = 0;

  mutable usize copiedCounter = 0;
  mutable usize movedCounter  = 0;
  mutable u64   calls;
  mutable u64   refers;

  Maybe<llvm::Function*>   asyncFn;
  Maybe<llvm::StructType*> asyncArgTy;

public:
  Function(Mod* mod, Identifier _name, Maybe<LinkNames> _namingInfo, Vec<GenericParameter*> _generics,
           ReturnType* returnType, Vec<Argument> _args, bool has_variadic_arguments, Maybe<FileRange> fileRange,
           const VisibilityInfo& _visibility_info, ir::Ctx* irCtx, bool isMemberFn = false,
           Maybe<llvm::GlobalValue::LinkageTypes> _linkage = None, Maybe<MetaInfo> _metaInfo = None);

  static Function* Create(Mod* mod, Identifier name, Maybe<LinkNames> _namingInfo, Vec<GenericParameter*> _generics,
                          ReturnType* return_type, Vec<Argument> args, bool has_variadic_args,
                          Maybe<FileRange> fileRange, const VisibilityInfo& visibilityInfo, ir::Ctx* irCtx,
                          Maybe<llvm::GlobalValue::LinkageTypes> linkage = None, Maybe<MetaInfo> metaInfo = None);

  useit Value*       call(ir::Ctx* irCtx, const Vec<llvm::Value*>& args, Maybe<String> localID, Mod* mod) override;
  useit virtual bool is_method() const;
  useit bool         has_variadic_args() const;
  useit Identifier   arg_name_at(u32 index) const;
  useit virtual Identifier get_name() const;
  useit virtual String     get_full_name() const;
  useit bool               is_accessible(const AccessInfo& req_info) const;
  useit VisibilityInfo     get_visibility() const;
  useit ir::Mod* get_module() const;
  useit llvm::Function* get_llvm_function();
  void                  set_active_block(usize index) const;
  useit Block*          get_block() const;
  useit Block*          get_first_block() const;
  useit usize           get_block_count() const;
  useit usize&          get_copied_counter();
  useit usize&          get_moved_counter();
  useit ir::LocalValue*   get_function_common_index();
  useit bool              isGeneric() const;
  useit bool              has_generic_parameter(const String& name) const;
  useit GenericParameter* get_generic_parameter(const String& name) const;
  useit bool              hasDefinitionRange() const;
  useit FileRange         getDefinitionRange() const;
  useit String            get_random_alloca_name() const;

  void update_overview() override;

  ~Function() override;
};

class GenericFunction : public Uniq, public EntityOverview {
private:
  Identifier                     name;
  Vec<ast::GenericAbstractType*> generics;
  ast::FunctionPrototype*        functionDefinition;
  Maybe<ast::PrerunExpression*>  constraint;
  Mod*                           parent;
  VisibilityInfo                 visibInfo;

  mutable Vec<GenericVariant<Function>> variants;

public:
  GenericFunction(Identifier name, Vec<ast::GenericAbstractType*> _generics, Maybe<ast::PrerunExpression*> constraint,
                  ast::FunctionPrototype* functionDef, Mod* parent, const VisibilityInfo& _visibInfo);

  ~GenericFunction() = default;

  useit Identifier get_name() const;
  useit usize      getTypeCount() const;
  useit usize      getVariantCount() const;
  useit Mod*       get_module() const;
  useit ast::GenericAbstractType* getGenericAt(usize index) const;
  useit VisibilityInfo            get_visibility() const;
  useit Function* fill_generics(Vec<ir::GenericToFill*> _types, ir::Ctx* irCtx, const FileRange& fileRange);
  useit bool      all_generics_have_default() const;
};

void function_return_handler(ir::Ctx* irCtx, ir::Function* fun, const FileRange& fileRange);
void destructor_caller(ir::Ctx* irCtx, ir::Function* fun);
void method_handler(ir::Ctx* irCtx, ir::Function* fun);
void destroy_locals_from(ir::Ctx* irCtx, ir::Block* block);

} // namespace qat::ir

#endif