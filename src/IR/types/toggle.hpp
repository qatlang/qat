#ifndef QAT_IR_TYPES_TOGGLE_HPP
#define QAT_IR_TYPES_TOGGLE_HPP

#include "../../utils/identifier.hpp"
#include "../../utils/visibility.hpp"
#include "./expanded_type.hpp"

#include <helpers/deque.hpp>

namespace qat::ast {
class DefineToggleType;
}

namespace qat::ir {

class ToggleType : public ExpandedType {
	Vec<Pair<Vec<Identifier>, Type*>> variants;
	usize                             underlyingTypeIndex = 0;

	Maybe<MetaInfo> metaInfo;
	usize           maxVariantByteSize = 0;

  public:
	ToggleType(Identifier _name, Vec<GenericArgument*> _generics, Vec<Pair<Vec<Identifier>, Type*>> _subTypes,
	           ir::OpaqueType* _opaqueType, ir::Mod* _parent, VisibilityInfo const& _visibility,
	           Maybe<MetaInfo> _metaInfo, ir::Ctx* irCtx);

	static ToggleType* create(Identifier name, Vec<GenericArgument*> generics,
	                          Vec<Pair<Vec<Identifier>, Type*>> variants, ir::OpaqueType* opaqueType, ir::Mod* parent,
	                          VisibilityInfo visibility, Maybe<MetaInfo> metaInfo, ir::Ctx* irCtx) {
		return std::construct_at(OwnNormal(ToggleType), std::move(name), std::move(generics), std::move(variants),
		                         opaqueType, parent, visibility, std::move(metaInfo), irCtx);
	}

	usize get_variant_count() const { return variants.size(); }

	usize get_max_variant_size() const { return maxVariantByteSize; }

	bool has_variant(String const& name) const {
		for (auto& it : variants) {
			for (auto& itName : it.first) {
				if (itName.value == name) {
					return true;
				}
			}
		}
		return false;
	}

	bool is_default_variant(String const& name) const {
		for (usize i = 0; i < variants.size(); i++) {
			for (auto& itName : variants[i].first) {
				if (itName.value == name) {
					return i == underlyingTypeIndex;
				}
			}
		}
		return false;
	}

	Type* get_variant_type_of(String const& name) const {
		for (auto& it : variants) {
			for (auto& itName : it.first) {
				if (itName.value == name) {
					return it.second;
				}
			}
		}
		return nullptr;
	}

	usize get_variant_index_of(String const& name) const {
		for (usize i = 0; i < variants.size(); i++) {
			for (auto& itName : variants[i].first) {
				if (itName.value == name) {
					return i;
				}
			}
		}
		return 0;
	}

	bool can_be_prerun() const final { return true; }

	bool can_be_prerun_generic() const final { return true; }

	bool has_prerun_default_value() const final { return true; }

	ir::PrerunValue* get_prerun_default_value(ir::Ctx* irCtx) final;

	Identifier const& get_variant_name_at(usize index) { return variants[index].first.front(); }

	Type* get_variant_type_at(usize index) const { return variants[index].second; }

	LinkNames get_link_names() const final;

	bool has_simple_copy() const final { return true; }

	bool has_simple_move() const final { return true; }

	TypeKind type_kind() const final { return TypeKind::TOGGLE; }

	String to_string() const final { return name.value; }
};

class GenericToggleType : public Uniq, public Mentionable {
	friend ast::DefineToggleType;

  private:
	Identifier                     name;
	Vec<ast::GenericAbstractType*> generics;
	ast::DefineToggleType*         defineToggleType;
	Mod*                           parent;
	VisibilityInfo                 visibility;
	ast::PrerunExpression*         constraint;

	std::set<String> variantNames;

	mutable Vec<GenericVariant<ToggleType>>   variants;
	mutable Deque<GenericVariant<OpaqueType>> opaqueVariants;

  public:
	GenericToggleType(Identifier _name, Vec<ast::GenericAbstractType*> _generics, ast::PrerunExpression* _constraint,
	                  ast::DefineToggleType* _defineToggleType, Mod* _parent, VisibilityInfo const& _visibInfo);

	static GenericToggleType* create(Identifier name, Vec<ast::GenericAbstractType*> generics,
	                                 ast::PrerunExpression* constraint, ast::DefineToggleType* defineToggleType,
	                                 Mod* parent, VisibilityInfo const& visibInfo) {
		return std::construct_at(OwnNormal(GenericToggleType), std::move(name), std::move(generics), constraint,
		                         defineToggleType, parent, visibInfo);
	}

	Identifier get_name() const { return name; }

	usize get_parameter_count() const { return generics.size(); }

	bool all_parameters_have_default() const;

	usize get_variant_count() const { return variants.size(); }

	Mod* get_module() const { return parent; }

	Type* fill_generics(Vec<ir::GenericToFill*>& types, ir::Ctx* irCtx, FileRangePtr range);

	ast::GenericAbstractType* get_generic_at(usize index) const { return generics[index]; }

	VisibilityInfo const& get_visibility() const { return visibility; }
};

} // namespace qat::ir

#endif
