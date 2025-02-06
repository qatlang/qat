#ifndef QAT_AST_TYPES_SUBTYPE_HPP
#define QAT_AST_TYPES_SUBTYPE_HPP

#include "../node.hpp"
#include "../types/qat_type.hpp"

namespace qat::ir {
class Type;
}

namespace qat::ast {

class SubType final : public Type {
	friend class MethodPrototype;

	Maybe<FileRange> skill;
	Maybe<FileRange> doneSkill;
	Vec<Identifier>  names;
	Type*            parentType;

  public:
	SubType(Maybe<FileRange> _skill, Maybe<FileRange> _doneSkill, Vec<Identifier> _names, Type* _parentType,
	        FileRange _fileRange)
	    : Type(std::move(_fileRange)), skill(std::move(_skill)), doneSkill(std::move(_doneSkill)),
	      names(std::move(_names)), parentType(_parentType) {}

	useit static SubType* create(Maybe<FileRange> skill, Maybe<FileRange> doneSkill, Vec<Identifier> names,
	                             Type* parent, FileRange fileRange) {
		return std::construct_at(OwnNormal(SubType), std::move(skill), std::move(doneSkill), std::move(names), parent,
		                         std::move(fileRange));
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
	                         EmitCtx* ctx) final;

	useit ir::Type* emit(EmitCtx* ctx) final;

	useit AstTypeKind type_kind() const final { return AstTypeKind::SUBTYPE; }

	useit Json to_json() const final {
		Vec<JsonValue> namesJSON;
		for (auto& id : names) {
			namesJSON.push_back(id);
		}
		return Json()
		    ._("hasSkill", skill.has_value())
		    ._("skillRange", skill.has_value() ? skill.value() : JsonValue())
		    ._("names", namesJSON)
		    ._("hasParentType", parentType != nullptr)
		    ._("parentType", parentType ? parentType->to_json() : JsonValue())
		    ._("fileRange", fileRange);
	}

	useit String to_string() const final {
		String nameStr;
		for (auto& id : names) {
			nameStr += ":" + id.value;
		}
		return (skill.has_value() ? "skill" : "") + (parentType ? parentType->to_string() : "") + nameStr;
	}
};

} // namespace qat::ast

#endif
