#ifndef QAT_AST_TYPES_FUTURE_TYPE_HPP
#define QAT_AST_TYPES_FUTURE_TYPE_HPP

#include "qat_type.hpp"

namespace qat::ast {

class FutureType final : public Type {
  private:
	ast::Type* subType;
	bool       isPacked;

  public:
	FutureType(bool _isPacked, ast::Type* _subType, FileRangePtr _fileRange)
	    : Type(_fileRange), subType(_subType), isPacked(_isPacked) {}

	static FutureType* create(bool isPacked, ast::Type* subType, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(FutureType), isPacked, subType, fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
	                         EmitCtx* ctx) final;

	Maybe<usize> get_type_bitsize(EmitCtx* ctx) const final;

	ir::Type* emit(EmitCtx* ctx) final;

	AstTypeKind type_kind() const final;

	String to_string() const final;
};

} // namespace qat::ast

#endif
