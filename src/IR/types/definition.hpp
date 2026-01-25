#ifndef QAT_IR_TYPES_DEFINITION_HPP
#define QAT_IR_TYPES_DEFINITION_HPP

#include "../../utils/file_range.hpp"
#include "../../utils/identifier.hpp"
#include "../../utils/qat_region.hpp"
#include "../../utils/visibility.hpp"
#include "../generic_variant.hpp"
#include "./expanded_type.hpp"

#include <helpers/maybe.hpp>
#include <helpers/string.hpp>
#include <helpers/vec.hpp>
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

	static TypeDefParent from_skill(Skill* skill) {
		return TypeDefParent{.kind = TypeDefParentKind::SKILL, .data = skill};
	}

	static TypeDefParent from_method_parent(MethodParent* parent) {
		return TypeDefParent{.kind = TypeDefParentKind::METHOD_PARENT, .data = parent};
	}

	String get_full_name() const {
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

	bool is_skill() const { return kind == TypeDefParentKind::SKILL; }

	bool is_method_parent() const { return kind == TypeDefParentKind::METHOD_PARENT; }

	Skill* as_skill() const { return (Skill*)data; }

	MethodParent* as_method_parent() const { return (MethodParent*)data; }
};

class DefinitionType : public ExpandedType {
	Maybe<TypeDefParent> parentEntity;
	Type*                subType;

  public:
	DefinitionType(Identifier _name, Type* _actualType, Vec<GenericArgument*> _generics,
	               Maybe<TypeDefParent> _methodParent, Mod* mod, const VisibilityInfo& _visibInfo);

	static DefinitionType* create(Identifier name, Type* actualType, Vec<GenericArgument*> generics,
	                              Maybe<TypeDefParent> methodParent, Mod* mod, const VisibilityInfo& visibInfo) {
		return std::construct_at(OwnNormal(DefinitionType), std::move(name), actualType, std::move(generics),
		                         methodParent, mod, visibInfo);
	}

	void setSubType(Type* _subType);

	bool has_custom_parent() const { return parentEntity.has_value(); }

	TypeDefParent const& get_custom_parent() const { return parentEntity.value(); }

	Identifier get_name() const;

	String get_full_name() const;

	Mod* get_module();

	Type* get_subtype();

	Type* get_non_definition_subtype();

	TypeKind type_kind() const final;

	LinkNames get_link_names() const final;

	String to_string() const final;

	bool is_type_sized() const final;

	void copy_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;

	void copy_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;

	void move_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;

	void move_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;

	void destroy_value(ir::Ctx* irCtx, ir::Value* instance, ir::Function* fun) final;

	bool can_be_prerun() const final { return subType->can_be_prerun(); }

	bool can_be_prerun_generic() const final { return subType->can_be_prerun_generic(); }

	Maybe<String> to_prerun_generic_string(ir::PrerunValue* constant) const final;

	Maybe<bool> equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const final;

	VisibilityInfo get_visibility() const;
};

class GenericDefinitionType : public Uniq, public Mentionable {
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
	                      Maybe<ast::PrerunExpression*> constraint, ast::TypeDefinition* defineStructType, Mod* parent,
	                      const VisibilityInfo& visibInfo);

	static GenericDefinitionType* create(Identifier name, Vec<ast::GenericAbstractType*> generics,
	                                     Maybe<ast::PrerunExpression*> constraint,
	                                     ast::TypeDefinition* defineStructType, Mod* parent,
	                                     const VisibilityInfo& visibInfo) {
		return std::construct_at(OwnNormal(GenericDefinitionType), std::move(name), std::move(generics), constraint,
		                         defineStructType, parent, visibInfo);
	}

	~GenericDefinitionType() {
		for (auto& it : variants) {
			it.clear_fill_types();
		}
	}

	Identifier get_name() const;

	usize get_generic_count() const;

	bool all_generics_have_defaults() const;

	usize get_variant_count() const;

	Mod* get_module() const;

	DefinitionType* fill_generics(Vec<ir::GenericToFill*>& types, ir::Ctx* irCtx, FileRangePtr range);

	ast::GenericAbstractType* get_generic_at(usize index) const;

	VisibilityInfo const& get_visibility() const;
};

} // namespace qat::ir

#endif
