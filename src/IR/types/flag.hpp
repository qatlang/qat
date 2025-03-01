#ifndef QAT_IR_TYPES_FLAG_HPP
#define QAT_IR_TYPES_FLAG_HPP

#include "../../utils/identifier.hpp"
#include "../../utils/qat_region.hpp"
#include "../../utils/visibility.hpp"
#include "./qat_type.hpp"

namespace qat::ir {

class Mod;

struct FlagVariant {
	Vec<Identifier> names;
	bool            isDefault;
};

class FlagType final : public Type {
	Identifier               name;
	Mod*                     parent;
	Vec<FlagVariant>         variants;
	Maybe<Vec<PrerunValue*>> values;
	UnsignedType*            underlyingType;
	FileRange                range;
	bool                     hasDefaultVariants;
	VisibilityInfo           visibility;

  public:
	FlagType(Identifier _name, Mod* _parent, Vec<FlagVariant> _variants, Maybe<Vec<PrerunValue*>> _values,
	         UnsignedType* _underlyingType, FileRange _range, VisibilityInfo _visibility);

	useit static FlagType* create(Identifier name, Mod* parent, Vec<FlagVariant> variants,
	                              Maybe<Vec<PrerunValue*>> values, UnsignedType* underlyingType, FileRange range,
	                              VisibilityInfo visibility) {
		return std::construct_at(OwnNormal(FlagType), std::move(name), parent, std::move(variants), std::move(values),
		                         underlyingType, std::move(range), std::move(visibility));
	}

	useit Identifier const& get_name() const { return name; }

	useit String get_full_name() const;

	useit Mod* get_module() const { return parent; }

	useit VisibilityInfo const& get_visibility() const { return visibility; }

	useit bool can_be_prerun() const final { return true; }

	useit bool can_be_prerun_generic() const final { return true; }

	useit bool has_prerun_default_value() const final { return true; }

	useit bool is_default_constructible() const final { return true; }

	useit bool is_copy_constructible() const final { return true; }

	useit bool is_copy_assignable() const final { return true; }

	useit bool is_move_constructible() const final { return true; }

	useit bool is_move_assignable() const final { return true; }

	useit bool is_destructible() const final { return true; }

	useit bool is_trivially_copyable() const final { return true; }

	useit bool is_trivially_movable() const final { return true; }

	useit PrerunValue* get_prerun_default_value(ir::Ctx* irCtx) final;

	useit Maybe<String> to_prerun_generic_string(ir::PrerunValue* val) const final;

	useit Maybe<bool> equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const final;

	useit bool is_type_sized() const final { return true; }

	useit bool has_default_variants() const { return hasDefaultVariants; }

	useit UnsignedType* get_underlying_type() const { return underlyingType; }

	useit Maybe<usize> get_index_of(String name) const;

	useit bool has_value_for(String name) const;

	useit PrerunValue* get_value_for(String name) const;

	useit TypeKind type_kind() const final { return TypeKind::flag; }

	useit String to_string() const final;
};

} // namespace qat::ir

#endif
