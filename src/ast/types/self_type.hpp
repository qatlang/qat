#ifndef QAT_AST_TYPES_SELF_TYPE_HPP
#define QAT_AST_TYPES_SELF_TYPE_HPP

#include "qat_type.hpp"

namespace qat::ast {

class SelfType final : public Type {
	friend class MethodPrototype;
	friend class OperatorPrototype;
	friend class DefineSkill;

	bool isJustType;

	bool canBeSelfInstance = false;
	bool isVarRef          = false;

  public:
	SelfType(bool _isJustType, FileRange _fileRange) : Type(_fileRange), isJustType(_isJustType) {}

	useit static SelfType* create(bool _isJustType, FileRange _fileRange) {
		return std::construct_at(OwnNormal(SelfType), _isJustType, _fileRange);
	}

	useit ir::Type*   emit(EmitCtx* ctx);
	useit Json        to_json() const;
	useit String      to_string() const;
	useit AstTypeKind type_kind() const { return AstTypeKind::SELF_TYPE; }
};

} // namespace qat::ast

#endif
