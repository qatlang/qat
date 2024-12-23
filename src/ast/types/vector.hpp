#ifndef QAT_AST_TYPES_VECTOR_HPP
#define QAT_AST_TYPES_VECTOR_HPP

#include "qat_type.hpp"
#include "type_kind.hpp"

namespace qat::ast {

class VectorType final : public Type {
	Type*             subType;
	PrerunExpression* count;
	Maybe<FileRange>  scalable;

  public:
	VectorType(Type* _subType, PrerunExpression* _count, Maybe<FileRange> _scalable, FileRange _fileRange)
	    : Type(_fileRange), subType(_subType), count(_count), scalable(_scalable) {}

	useit static VectorType* create(Type* _subType, PrerunExpression* _count, Maybe<FileRange> _scalable,
	                                FileRange _fileRange) {
		return std::construct_at(OwnNormal(VectorType), _subType, _count, _scalable, _fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
	                         EmitCtx* ctx) final;

	useit ir::Type* emit(EmitCtx* ctx);

	AstTypeKind type_kind() const final { return AstTypeKind::VECTOR; }

	Json to_json() const final;

	String to_string() const final;
};

} // namespace qat::ast

#endif
