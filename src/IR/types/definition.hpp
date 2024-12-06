#ifndef QAT_IR_TYPES_DEFINITION_HPP
#define QAT_IR_TYPES_DEFINITION_HPP

#include "../../utils/file_range.hpp"
#include "../../utils/helpers.hpp"
#include "../../utils/identifier.hpp"
#include "../../utils/visibility.hpp"
#include "../entity_overview.hpp"
#include "../generic_variant.hpp"
#include "./expanded_type.hpp"

#include <llvm/IR/LLVMContext.h>

namespace qat::ast {
class GenericAbstractType;
class TypeDefinition;
} // namespace qat::ast

namespace qat::ir {

class Mod;

class DefinitionType : public ExpandedType, public EntityOverview {
private:
  Type* subType;

public:
  DefinitionType(Identifier _name, Type* _actualType, Vec<GenericArgument*> _generics, Mod* mod,
                 const VisibilityInfo& _visibInfo);

  void setSubType(Type* _subType);

  useit Identifier get_name() const;
  useit String     get_full_name() const;
  useit Mod*       get_module();
  useit Type*      get_subtype();
  useit TypeKind   type_kind() const final;
  useit LinkNames  get_link_names() const final;
  useit String     to_string() const final;
  useit bool       is_expanded() const final;
  useit bool       is_type_sized() const final;

  useit bool is_trivially_copyable() const final;
  useit bool is_trivially_movable() const final;
  useit bool is_copy_constructible() const final;
  useit bool is_copy_assignable() const final;
  useit bool is_move_constructible() const final;
  useit bool is_move_assignable() const final;
  useit bool is_destructible() const final;

  void copy_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;
  void copy_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;
  void move_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;
  void move_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;
  void destroy_value(ir::Ctx* irCtx, ir::Value* instance, ir::Function* fun) final;
  void update_overview() final;

  useit bool can_be_prerun() const final { return subType->can_be_prerun(); }
  useit bool can_be_prerun_generic() const final { return subType->can_be_prerun_generic(); }
  useit Maybe<String> to_prerun_generic_string(ir::PrerunValue* constant) const final;
  useit Maybe<bool> equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const final;

  useit VisibilityInfo get_visibility() const;
};

class GenericDefinitionType : public Uniq, public EntityOverview {
private:
  Identifier                     name;
  Vec<ast::GenericAbstractType*> generics;
  ast::TypeDefinition*           defineTypeDef;
  Mod*                           parent;
  VisibilityInfo                 visibility;

  Maybe<ast::PrerunExpression*> constraint;

  mutable Vec<GenericVariant<DefinitionType>> variants;

public:
  GenericDefinitionType(Identifier name, Vec<ast::GenericAbstractType*> generics,
                        Maybe<ast::PrerunExpression*> constraint, ast::TypeDefinition* defineCoreType, Mod* parent,
                        const VisibilityInfo& visibInfo);

  ~GenericDefinitionType() = default;

  useit Identifier      get_name() const;
  useit usize           get_generic_count() const;
  useit bool            all_generics_have_defaults() const;
  useit usize           get_variant_count() const;
  useit Mod*            get_module() const;
  useit DefinitionType* fill_generics(Vec<ir::GenericToFill*>& types, ir::Ctx* irCtx, FileRange range);

  useit ast::GenericAbstractType* get_generic_at(usize index) const;
  useit VisibilityInfo            get_visibility() const;
};

} // namespace qat::ir

#endif
