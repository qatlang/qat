#ifndef QAT_IR_TYPES_FLAG_HPP
#define QAT_IR_TYPES_FLAG_HPP

#include "../../utils/identifier.hpp"
#include "../../utils/mentionable.hpp"
#include "../../utils/qat_region.hpp"
#include "../../utils/visibility.hpp"
#include "./qat_type.hpp"

namespace qat::ast {
struct PatternFlag;
}

namespace qat::ir {

class Mod;

struct FlagVariant {
	Vec<Identifier> names;
	bool            isDefault;
};

class FlagType final : public Type, public Mentionable {
	friend struct ast::PatternFlag;

	Identifier               name;
	Mod*                     parent;
	Vec<FlagVariant>         variants;
	Maybe<Vec<PrerunValue*>> values;
	UnsignedType*            underlyingType;
	FileRangePtr             range;
	bool                     hasDefaultVariants;
	VisibilityInfo           visibility;

  public:
	FlagType(Identifier _name, Mod* _parent, Vec<FlagVariant> _variants, Maybe<Vec<PrerunValue*>> _values,
	         UnsignedType* _underlyingType, FileRangePtr _range, VisibilityInfo _visibility);

	static FlagType* create(Identifier name, Mod* parent, Vec<FlagVariant> variants, Maybe<Vec<PrerunValue*>> values,
	                        UnsignedType* underlyingType, FileRangePtr range, VisibilityInfo visibility) {
		return std::construct_at(OwnNormal(FlagType), std::move(name), parent, std::move(variants), std::move(values),
		                         underlyingType, std::move(range), std::move(visibility));
	}

	Identifier const& get_name() const { return name; }

	String get_full_name() const;

	Mod* get_module() const { return parent; }

	VisibilityInfo const& get_visibility() const { return visibility; }

	bool can_be_prerun() const final { return true; }

	bool can_be_prerun_generic() const final { return true; }

	bool has_simple_copy() const final { return true; }

	bool has_simple_move() const final { return true; }

	bool has_prerun_default_value() const final { return true; }

	PrerunValue* get_prerun_default_value(ir::Ctx* irCtx) final;

	Maybe<String> to_prerun_generic_string(ir::PrerunValue* val) const final;

	Maybe<bool> equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const final;

	bool is_type_sized() const final { return true; }

	bool has_default_variants() const { return hasDefaultVariants; }

	UnsignedType* get_underlying_type() const { return underlyingType; }

	Maybe<usize> get_index_of(String name) const;

	bool has_value_for(String name) const;

	PrerunValue* get_value_for(String name) const;

	TypeKind type_kind() const final { return TypeKind::FLAG; }

	String to_string() const final;
};

} // namespace qat::ir

#endif
