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
	SelfType(bool _isJustType, FileRangePtr _fileRange) : Type(_fileRange), isJustType(_isJustType) {}

	static SelfType* create(bool _isJustType, FileRangePtr _fileRange) {
		return std::construct_at(OwnNormal(SelfType), _isJustType, _fileRange);
	}

	ir::Type* emit(EmitCtx* ctx);

	String to_string() const;

	AstTypeKind type_kind() const { return AstTypeKind::SELF_TYPE; }
};

} // namespace qat::ast

#endif
