#ifndef QAT_IR_TYPES_TOGGLE_HPP
#define QAT_IR_TYPES_TOGGLE_HPP

#include "../../utils/identifier.hpp"
#include "../../utils/visibility.hpp"
#include "./expanded_type.hpp"

namespace qat::ast {
class DefineToggleType;
}

namespace qat::ir {

class ToggleType : public ExpandedType, public EntityOverview {
	Vec<Pair<Vec<Identifier>, Type*>> variants;
	usize                             underlyingTypeIndex = 0;

	Maybe<MetaInfo> metaInfo;
	usize           maxVariantByteSize = 0;

  public:
	ToggleType(Identifier _name, Vec<GenericArgument*> _generics, Vec<Pair<Vec<Identifier>, Type*>> _subTypes,
	           ir::OpaqueType* _opaqueType, ir::Mod* _parent, VisibilityInfo const& _visibility,
	           Maybe<MetaInfo> _metaInfo, ir::Ctx* irCtx);

	useit static ToggleType* create(Identifier name, Vec<GenericArgument*> generics,
	                                Vec<Pair<Vec<Identifier>, Type*>> variants, ir::OpaqueType* opaqueType,
	                                ir::Mod* parent, VisibilityInfo visibility, Maybe<MetaInfo> metaInfo,
	                                ir::Ctx* irCtx) {
		return std::construct_at(OwnNormal(ToggleType), std::move(name), std::move(generics), std::move(variants),
		                         opaqueType, parent, visibility, std::move(metaInfo), irCtx);
	}

	useit usize get_variant_count() const { return variants.size(); }

	useit usize get_max_variant_size() const { return maxVariantByteSize; }

	useit bool has_variant(String const& name) const {
		for (auto& it : variants) {
			for (auto& itName : it.first) {
				if (itName.value == name) {
					return true;
				}
			}
		}
		return false;
	}

	useit bool is_default_variant(String const& name) const {
		for (usize i = 0; i < variants.size(); i++) {
			for (auto& itName : variants[i].first) {
				if (itName.value == name) {
					return i == underlyingTypeIndex;
				}
			}
		}
		return false;
	}

	useit Type* get_variant_type_of(String const& name) const {
		for (auto& it : variants) {
			for (auto& itName : it.first) {
				if (itName.value == name) {
					return it.second;
				}
			}
		}
		return nullptr;
	}

	useit usize get_variant_index_of(String const& name) const {
		for (usize i = 0; i < variants.size(); i++) {
			for (auto& itName : variants[i].first) {
				if (itName.value == name) {
					return i;
				}
			}
		}
		return 0;
	}

	useit bool can_be_prerun() const final { return true; }

	useit bool can_be_prerun_generic() const final { return true; }

	useit bool has_prerun_default_value() const final { return true; }

	useit ir::PrerunValue* get_prerun_default_value(ir::Ctx* irCtx) final;

	useit Identifier const& get_variant_name_at(usize index) { return variants[index].first.front(); }

	useit Type* get_variant_type_at(usize index) const { return variants[index].second; }

	useit LinkNames get_link_names() const final;

	useit bool has_simple_copy() const final { return true; }

	useit bool has_simple_move() const final { return true; }

	useit TypeKind type_kind() const final { return TypeKind::TOGGLE; }

	useit String to_string() const final { return name.value; }

	void update_overview() final;
};

class GenericToggleType : public Uniq, public EntityOverview {
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

	useit static GenericToggleType* create(Identifier name, Vec<ast::GenericAbstractType*> generics,
	                                       ast::PrerunExpression* constraint, ast::DefineToggleType* defineToggleType,
	                                       Mod* parent, VisibilityInfo const& visibInfo) {
		return std::construct_at(OwnNormal(GenericToggleType), std::move(name), std::move(generics), constraint,
		                         defineToggleType, parent, visibInfo);
	}

	useit Identifier get_name() const { return name; }

	useit usize get_parameter_count() const { return generics.size(); }

	useit bool all_parameters_have_default() const;

	useit usize get_variant_count() const { return variants.size(); }

	useit Mod* get_module() const { return parent; }

	useit Type* fill_generics(Vec<ir::GenericToFill*>& types, ir::Ctx* irCtx, FileRange range);

	useit ast::GenericAbstractType* get_generic_at(usize index) const { return generics[index]; }

	useit VisibilityInfo const& get_visibility() const { return visibility; }

	void update_overview() final;
};

} // namespace qat::ir

#endif
