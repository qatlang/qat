#ifndef QAT_AST_TYPES_POLYMORPH_HPP
#define QAT_AST_TYPES_POLYMORPH_HPP

#include "../node.hpp"
#include "./mark_owner.hpp"
#include "./qat_type.hpp"
#include "./type_kind.hpp"

namespace qat::ir {
class Skill;
}

namespace qat::ast {

struct SkillEntity {
	u32             relative = 0;
	Vec<Identifier> names;
	FileRange       range;
	// TODO - Support generic skills

	SkillEntity(u32 _relative, Vec<Identifier> _names, FileRange _range)
	    : relative(_relative), names(std::move(_names)), range(std::move(_range)) {}

	useit static SkillEntity* create(u32 relative, Vec<Identifier> names, FileRange range) {
		return std::construct_at(OwnNormal(SkillEntity), relative, std::move(names), std::move(range));
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent, EmitCtx* ctx);

	useit ir::Skill* find(EmitCtx* ctx) const;

	useit Json to_json() const {
		Vec<JsonValue> namesJSON;
		for (auto& id : names) {
			namesJSON.push_back(id);
		}
		return Json()._("relative", relative)._("names", namesJSON)._("range", range);
	}

	useit String to_string() const {
		String result;
		for (usize i = 0; i < relative; i++) {
			result += "up:";
		}
		for (usize i = 0; i < names.size(); i++) {
			result += names[i].value;
			if (i != (names.size())) {
				result += ':';
			}
		}
		return result;
	}
};

class PolymorphType final : public Type {
	bool              isTyped;
	Vec<SkillEntity*> skills;
	MarkOwnType       ownType;
	Maybe<FileRange>  ownRange;

  public:
	PolymorphType(bool _isTyped, Vec<SkillEntity*> _skills, MarkOwnType _ownType, Maybe<FileRange> _ownRange,
	              FileRange _range)
	    : Type(std::move(_range)), isTyped(_isTyped), skills(std::move(_skills)), ownType(_ownType),
	      ownRange(std::move(_ownRange)) {}

	useit static PolymorphType* create(bool isTyped, Vec<SkillEntity*> skills, MarkOwnType ownType,
	                                   Maybe<FileRange> ownRange, FileRange range) {
		return std::construct_at(OwnNormal(PolymorphType), isTyped, std::move(skills), ownType, std::move(ownRange),
		                         std::move(range));
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent, EmitCtx* ctx) {
		for (auto* sk : skills) {
			UPDATE_DEPS(sk);
		}
	}

	useit Maybe<usize> get_type_bitsize(EmitCtx* ctx) const final;

	useit ir::Type* emit(EmitCtx* ctx);

	useit AstTypeKind type_kind() const final { return AstTypeKind::POLYMORPH; }

	useit Json to_json() const final {
		Vec<JsonValue> skillsJSON;
		for (auto* sk : skills) {
			skillsJSON.push_back(sk->to_json());
		}
		return Json()
		    ._("typeKind", "polymorph")
		    ._("isTyped", isTyped)
		    ._("skills", skillsJSON)
		    ._("markOwner", mark_owner_to_string(ownType))
		    ._("hasOwnRange", ownRange.has_value())
		    ._("ownRange", ownRange.has_value() ? ownRange.value() : JsonValue());
	}

	useit String to_string() const final {
		String skillStr;
		for (usize i = 0; i < skills.size(); i++) {
			skillStr += skills[i]->to_string();
			if (i != (skills.size() - 1)) {
				skillStr += " + ";
			}
		}
		return (isTyped ? "poly:[type, " : "poly:[") + skillStr +
		       (ownType != MarkOwnType::anonymous ? (", " + mark_owner_to_string(ownType)) : "") + "]";
	}
};

} // namespace qat::ast

#endif
