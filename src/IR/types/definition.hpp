#ifndef QAT_IR_TYPES_DEFINITION_HPP
#define QAT_IR_TYPES_DEFINITION_HPP

#include "../../utils/file_range.hpp"
#include "../../utils/helpers.hpp"
#include "../../utils/identifier.hpp"
#include "../../utils/qat_region.hpp"
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
class DoneSkill;

enum class TypeDefParentKind { SKILL, METHOD_PARENT };

struct TypeDefParent {
	TypeDefParentKind kind;
	void*             data;

	useit static TypeDefParent from_skill(Skill* skill) {
		return TypeDefParent{.kind = TypeDefParentKind::SKILL, .data = skill};
	}

	useit static TypeDefParent from_method_parent(MethodParent* parent) {
		return TypeDefParent{.kind = TypeDefParentKind::METHOD_PARENT, .data = parent};
	}

	useit String get_full_name() const {
		if (kind == TypeDefParentKind::SKILL) {
			return ((ir::Skill*)data)->get_full_name();
		} else {
			auto mem = (ir::MethodParent*)data;
			if (mem->is_done_skill()) {
				return mem->as_done_skill()->get_full_name();
			} else {
				return mem->as_expanded()->get_full_name();
			}
		}
	}

	useit bool is_skill() const { return kind == TypeDefParentKind::SKILL; }

	useit bool is_method_parent() const { return kind == TypeDefParentKind::METHOD_PARENT; }

	useit Skill* as_skill() const { return (Skill*)data; }

	useit MethodParent* as_method_parent() const { return (MethodParent*)data; }
};

class DefinitionType : public ExpandedType, public EntityOverview {
	Maybe<TypeDefParent> parentEntity;
	Type*                subType;

  public:
	DefinitionType(Identifier _name, Type* _actualType, Vec<GenericArgument*> _generics,
	               Maybe<TypeDefParent> _methodParent, Mod* mod, const VisibilityInfo& _visibInfo);

	useit static DefinitionType* create(Identifier name, Type* actualType, Vec<GenericArgument*> generics,
	                                    Maybe<TypeDefParent> methodParent, Mod* mod, const VisibilityInfo& visibInfo) {
		return std::construct_at(OwnNormal(DefinitionType), std::move(name), actualType, std::move(generics),
		                         methodParent, mod, visibInfo);
	}

	void setSubType(Type* _subType);

	useit bool has_custom_parent() const { return parentEntity.has_value(); }

	useit TypeDefParent const& get_custom_parent() const { return parentEntity.value(); }

	useit Identifier get_name() const;

	useit String get_full_name() const;

	useit Mod* get_module();

	useit Type* get_subtype();

	useit Type* get_non_definition_subtype();

	useit TypeKind type_kind() const final;

	useit LinkNames get_link_names() const final;

	useit String to_string() const final;

	useit bool is_expanded() const final;

	useit bool is_type_sized() const final;

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

	useit static GenericDefinitionType* create(Identifier name, Vec<ast::GenericAbstractType*> generics,
	                                           Maybe<ast::PrerunExpression*> constraint,
	                                           ast::TypeDefinition* defineCoreType, Mod* parent,
	                                           const VisibilityInfo& visibInfo) {
		return std::construct_at(OwnNormal(GenericDefinitionType), std::move(name), std::move(generics), constraint,
		                         defineCoreType, parent, visibInfo);
	}

	~GenericDefinitionType() {
		for (auto& it : variants) {
			it.clear_fill_types();
		}
	}

	useit Identifier get_name() const;

	useit usize get_generic_count() const;

	useit bool all_generics_have_defaults() const;

	useit usize get_variant_count() const;

	useit Mod* get_module() const;

	useit DefinitionType* fill_generics(Vec<ir::GenericToFill*>& types, ir::Ctx* irCtx, FileRange range);

	useit ast::GenericAbstractType* get_generic_at(usize index) const;

	useit VisibilityInfo const& get_visibility() const;
};

} // namespace qat::ir

#endif
