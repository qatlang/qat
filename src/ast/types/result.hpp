#ifndef QAT_AST_TYPES_RESULT_HPP
#define QAT_AST_TYPES_RESULT_HPP

#include "./qat_type.hpp"

namespace qat::ast {

class ResultType final : public Type {
	ast::Type* validType;
	ast::Type* errorType;
	bool       isPacked;

  public:
	ResultType(ast::Type* _validType, ast::Type* _errorType, bool _isPacked, FileRangePtr _fileRange)
	    : Type(_fileRange), validType(_validType), errorType(_errorType), isPacked(_isPacked) {}

	static ResultType* create(ast::Type* _validType, ast::Type* _errorType, bool _isPacked, FileRangePtr _fileRange) {
		return std::construct_at(OwnNormal(ResultType), _validType, _errorType, _isPacked, _fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
	                         EmitCtx* ctx) final;

	ir::Type* emit(EmitCtx* ctx) final;

	AstTypeKind type_kind() const final;

	String to_string() const final;
};

} // namespace qat::ast

#endif
