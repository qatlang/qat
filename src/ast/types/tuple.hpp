#ifndef QAT_AST_TYPES_TUPLE_HPP
#define QAT_AST_TYPES_TUPLE_HPP

#include "../../IR/context.hpp"
#include "./qat_type.hpp"

#include <vector>

namespace qat::ast {

class TupleType final : public Type {
  private:
	Vec<Type*> types;
	bool       isPacked;

  public:
	TupleType(Vec<Type*> _types, bool _isPacked, FileRange _fileRange)
	    : Type(_fileRange), types(_types), isPacked(_isPacked) {}

	useit static TupleType* create(Vec<Type*> _types, bool _isPacked, FileRange _fileRange) {
		return std::construct_at(OwnNormal(TupleType), _types, _isPacked, _fileRange);
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
