#ifndef QAT_AST_TYPES_SUBTYPE_HPP
#define QAT_AST_TYPES_SUBTYPE_HPP

#include "../types/qat_type.hpp"

namespace qat::ir {
class Type;
}

namespace qat::ast {

class SubType final : public Type {
	friend class MethodPrototype;
	friend class DefineSkill;

	Maybe<FileRangePtr> skill;
	Maybe<FileRangePtr> doneSkill;
	Vec<Identifier>     names;
	Type*               parentType;

  public:
	SubType(Maybe<FileRangePtr> _skill, Maybe<FileRangePtr> _doneSkill, Vec<Identifier> _names, Type* _parentType,
	        FileRangePtr _fileRange)
	    : Type(std::move(_fileRange)), skill(std::move(_skill)), doneSkill(std::move(_doneSkill)),
	      names(std::move(_names)), parentType(_parentType) {}

	useit static SubType* create(Maybe<FileRangePtr> skill, Maybe<FileRangePtr> doneSkill, Vec<Identifier> names,
	                             Type* parent, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(SubType), std::move(skill), std::move(doneSkill), std::move(names), parent,
		                         fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
	                         EmitCtx* ctx) final;

	useit ir::Type* emit(EmitCtx* ctx) final;

	useit AstTypeKind type_kind() const final { return AstTypeKind::SUBTYPE; }

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
