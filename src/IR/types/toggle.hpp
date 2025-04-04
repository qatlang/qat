#ifndef QAT_IR_TYPES_TOGGLE_HPP
#define QAT_IR_TYPES_TOGGLE_HPP

#include "../../utils/identifier.hpp"
#include "../../utils/visibility.hpp"
#include "./expanded_type.hpp"

namespace qat::ir {

class ToggleType : public ExpandedType {
	Vec<Pair<Identifier, Type*>> variants;
	Maybe<MetaInfo>              metaInfo;

  public:
	ToggleType(Identifier _name, Vec<Pair<Identifier, Type*>> _subTypes, ir::Mod* _parent, VisibilityInfo _visibility,
	           Maybe<MetaInfo> _metaInfo, ir::Ctx* irCtx);

	useit static ToggleType* create(Identifier name, Vec<Pair<Identifier, Type*>> variants, ir::Mod* parent,
	                                VisibilityInfo visibility, Maybe<MetaInfo> metaInfo, ir::Ctx* irCtx) {
		return std::construct_at(OwnNormal(ToggleType), name, variants, parent, visibility, metaInfo, irCtx);
	}

	useit usize get_variant_count() const { return variants.size(); }

	useit Type* get_type_of(String const& name) const {
		for (auto& it : variants) {
			if (it.first.value == name) {
				return it.second;
			}
		}
		return nullptr;
	}

	useit LinkNames get_link_names() const final;

	useit bool has_simple_copy() const final { return true; }

	useit bool has_simple_move() const final { return true; }

	useit TypeKind type_kind() const final { return TypeKind::TOGGLE; }

	useit String to_string() const final { return name.value; }
};

} // namespace qat::ir

#endif
