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
  String name;

public:
  LocalValue(String name, ir::Type* type, bool is_variable, Function* fun, FileRange fileRange);

  useit static inline LocalValue* get(String name, ir::Type* type, bool isVar, Function* fn, FileRange fileRange) {
    return new LocalValue(name, type, isVar, fn, fileRange);
  }

  ~LocalValue() final = default;

  useit String get_name() const;
  useit llvm::AllocaInst* get_alloca() const;
  useit FileRange         get_file_range() const;
  useit ir::Value* to_new_ir_value() const;
};

class Block : public Uniq {
  friend Method;

private:
  String              name;
  llvm::BasicBlock*   bb;
  Vec<LocalValue*>    values;
  Block*              parent = nullptr;
  Vec<Block*>         children;
  Function*           fn;
  usize               index;
  Maybe<usize>        active;
  mutable Vec<String> movedValues;
  mutable bool        isGhost = false;
  mutable bool        hasGive = false;
  mutable bool        hasTodo = false;

  Block* prevBlock = nullptr;
  Block* nextBlock = nullptr;

public:
  Maybe<FileRange> fileRange;

  Block(Function* _fn, Block* _parent);

  ~Block() = default;

  useit inline String            get_name() const { return name; }
  useit inline llvm::BasicBlock* get_bb() const { return bb; }

  useit inline bool   has_previous_block() const { return prevBlock != nullptr; }
  useit inline Block* get_previous_block() const { return prevBlock; }
  useit inline bool   has_next_block() const { return nextBlock != nullptr; }
  useit inline Block* get_next_block() const { return nextBlock; }

  useit inline bool        has_parent() const { return parent != nullptr; }
  useit inline Block*      get_parent() const { return parent; }
  useit inline Function*   get_fn() const { return fn; }
  useit bool               has_value(const String& name) const;
  useit LocalValue*        get_value(const String& name) const;
  useit inline LocalValue* new_value(const String& name, ir::Type* type, bool isVar, FileRange fileRange) {
    values.push_back(LocalValue::get(name, type, isVar, fn, fileRange));
    return values.back();
  }
  useit bool is_moved(const String& locID) const;
  useit bool has_give_in_all_control_paths() const;

  useit inline bool   has_todo() const { return hasTodo; }
  useit inline Block* get_active() {
    if (active) {
      return children.at(active.value())->get_active();
    } else {
      return this;
    }
  }

  useit inline Vec<LocalValue*>& get_locals() { return values; }
  useit inline Maybe<FileRange>  get_file_range() const {
    if (fileRange.has_value()) {
      return fileRange;
    }
    if (parent && parent->fileRange.has_value()) {
      return parent->fileRange;
    }
    return None;
  }

  inline void link_previous_block(Block* block) {
    prevBlock        = block;
    block->nextBlock = this;
  }
  inline void set_file_range(FileRange _fileRange) { fileRange = _fileRange; }
  inline void set_has_give() const { hasGive = true; }
  inline void set_has_todo() const { hasTodo = true; }
  inline void add_moved_value(String locID) const { movedValues.push_back(std::move(locID)); }

  void set_active(llvm::IRBuilder<>& builder);
  void collect_all_local_values_so_far(Vec<LocalValue*>& vals) const;
  void collect_locals_from(Vec<LocalValue*>& vals) const;
  void destroy_locals(ast::EmitCtx* ctx);
  void output_local_overview(Vec<JsonValue>& jsonVals);
};

// Function represents a normal function in the language
class Function : public Value, public Uniq, public EntityOverview {
  friend class Block;

protected:
  Identifier            name;
  LinkNames             namingInfo;
  String                linkingName;
  Vec<GenericArgument*> generics;
  Mod*                  mod;
  Vec<Argument>         arguments;
  VisibilityInfo        visibilityInfo;
  Maybe<FileRange>      fileRange;
  bool                  hasVariadicArguments;
  bool                  isInline;
  Vec<Block*>           blocks;
  ir::LocalValue*       strComparisonIndex = nullptr;
  Maybe<MetaInfo>       metaInfo;
  Ctx*                  ctx;

  mutable u64   localNameCounter = 0;
  mutable usize activeBlock      = 0;

public:
  Function(Mod* mod, Identifier _name, Maybe<LinkNames> _namingInfo, Vec<GenericArgument*> _generics, bool isInline,
           ReturnType* returnType, Vec<Argument> _args, Maybe<FileRange> fileRange,
           const VisibilityInfo& _visibility_info, ir::Ctx* irCtx, bool _isMemberFn = false,
           Maybe<llvm::GlobalValue::LinkageTypes> _linkage = None, Maybe<MetaInfo> _metaInfo = None);

  static Function* Create(Mod* mod, Identifier name, Maybe<LinkNames> _namingInfo, Vec<GenericArgument*> _generics,
                          bool isInline, ReturnType* return_type, Vec<Argument> args, Maybe<FileRange> fileRange,
                          const VisibilityInfo& visibilityInfo, ir::Ctx* irCtx,
                          Maybe<llvm::GlobalValue::LinkageTypes> linkage = None, Maybe<MetaInfo> metaInfo = None);

  useit Value*             call(Ctx* irCtx, const Vec<llvm::Value*>& args, Maybe<String> localID, Mod* mod) override;
  useit virtual bool       is_method() const { return false; }
  useit inline bool        has_variadic_args() const { return hasVariadicArguments; }
  useit inline Identifier  arg_name_at(u32 index) const { return arguments[index].get_name(); }
  useit virtual Identifier get_name() const { return name; }
  useit virtual String     get_full_name() const;
  useit inline bool is_accessible(const AccessInfo& req_info) const { return visibilityInfo.is_accessible(req_info); }
  useit inline VisibilityInfo const& get_visibility() const { return visibilityInfo; }
  useit inline ir::Mod*              get_module() const { return mod; }
  useit inline llvm::Function*       get_llvm_function() { return llvm::cast<llvm::Function>(ll); }
  useit inline Block*                get_block() const { return blocks.at(activeBlock)->get_active(); }
  useit inline Block*                get_first_block() const { return blocks[0]; }
  useit inline usize                 get_block_count() const { return blocks.size(); }
  useit inline bool                  is_inline() const { return isInline; }
  useit LocalValue*                  get_str_comparison_index();
  useit inline bool                  is_generic() const { return !generics.empty(); }
  useit inline bool                  has_generic_parameter(const String& name) const {
    for (auto* gen : generics) {
      if (gen->get_name().value == name) {
        return true;
      }
    }
    return false;
  }
  useit inline GenericArgument* get_generic_parameter(const String& name) const {
    for (auto* gen : generics) {
      if (gen->get_name().value == name) {
        return gen;
      }
    }
    return nullptr;
  }
  useit inline bool      has_definition_range() const { return fileRange.has_value(); }
  useit inline FileRange get_definition_range() const { return fileRange.value(); }

  useit inline String get_random_alloca_name() const {
    localNameCounter++;
    return std::to_string(localNameCounter) + "_new";
  }

  inline void set_active_block(usize index) const { activeBlock = index; }

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

  void update_overview() final;
};

void function_return_handler(ir::Ctx* irCtx, ir::Function* fun, const FileRange& fileRange);
void destructor_caller(ir::Ctx* irCtx, ir::Function* fun);
void method_handler(ir::Ctx* irCtx, ir::Function* fun);
void destroy_locals_from(ir::Ctx* irCtx, ir::Block* block);

} // namespace qat::ir

#endif
