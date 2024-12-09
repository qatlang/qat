#ifndef QAT_AST_TYPES_FUTURE_TYPE_HPP
#define QAT_AST_TYPES_FUTURE_TYPE_HPP

#include "qat_type.hpp"

namespace qat::ast {

class FutureType final : public Type {
  private:
	ast::Type* subType;
	bool       isPacked;

  public:
	FutureType(bool _isPacked, ast::Type* _subType, FileRange _fileRange)
	    : Type(_fileRange), subType(_subType), isPacked(_isPacked) {}

	useit static FutureType* create(bool isPacked, ast::Type* subType, FileRange fileRange) {
		return std::construct_at(OwnNormal(FutureType), isPacked, subType, fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
	                         EmitCtx* ctx) final;

	Maybe<usize> getTypeSizeInBits(EmitCtx* ctx) const final;

	useit ir::Type*   emit(EmitCtx* ctx) final;
	useit AstTypeKind type_kind() const final;
	useit Json        to_json() const final;
	useit String      to_string() const final;
};

} // namespace qat::ast

#endif
