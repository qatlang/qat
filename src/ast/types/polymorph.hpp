#ifndef QAT_AST_TYPES_POLYMORPH_HPP
#define QAT_AST_TYPES_POLYMORPH_HPP

#include "../node.hpp"
#include "../skill_entity.hpp"
#include "./mark_owner.hpp"
#include "./qat_type.hpp"
#include "./type_kind.hpp"

namespace qat::ir {
class Skill;
}

namespace qat::ast {

class PolymorphType final : public Type {
	bool             isTyped;
	Vec<SkillEntity> skills;
	PtrOwnType       ownType;
	Maybe<FileRange> ownRange;

  public:
	PolymorphType(bool _isTyped, Vec<SkillEntity> _skills, PtrOwnType _ownType, Maybe<FileRange> _ownRange,
	              FileRange _range)
	    : Type(std::move(_range)), isTyped(_isTyped), skills(std::move(_skills)), ownType(_ownType),
	      ownRange(std::move(_ownRange)) {}

	useit static PolymorphType* create(bool isTyped, Vec<SkillEntity> skills, PtrOwnType ownType,
	                                   Maybe<FileRange> ownRange, FileRange range) {
		return std::construct_at(OwnNormal(PolymorphType), isTyped, std::move(skills), ownType, std::move(ownRange),
		                         std::move(range));
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent, EmitCtx* ctx) {
		for (auto& sk : skills) {
			sk.update_dependencies(phase, ir::DependType::complete, ent, ctx);
		}
	}

	useit Maybe<usize> get_type_bitsize(EmitCtx* ctx) const final;

	useit ir::Type* emit(EmitCtx* ctx);

	useit AstTypeKind type_kind() const final { return AstTypeKind::POLYMORPH; }

	useit Json to_json() const final {
		Vec<JsonValue> skillsJSON;
		for (auto sk : skills) {
			skillsJSON.push_back(sk.to_json());
		}
		return Json()
		    ._("typeKind", "polymorph")
		    ._("isTyped", isTyped)
		    ._("skills", skillsJSON)
		    ._("ptrOwner", ptr_owner_to_string(ownType))
		    ._("hasOwnRange", ownRange.has_value())
		    ._("ownRange", ownRange.has_value() ? ownRange.value() : JsonValue());
	}

	useit String to_string() const final {
		String skillStr;
		for (usize i = 0; i < skills.size(); i++) {
			skillStr += skills[i].to_string();
			if (i != (skills.size() - 1)) {
				skillStr += " + ";
			}
		}
		return (isTyped ? "poly:[type, " : "poly:[") + skillStr +
		       (ownType != PtrOwnType::anonymous ? (", " + ptr_owner_to_string(ownType)) : "") + "]";
	}
};

} // namespace qat::ast

#endif
