#ifndef QAT_AST_TYPES_VECTOR_HPP
#define QAT_AST_TYPES_VECTOR_HPP

#include "./qat_type.hpp"
#include "./type_kind.hpp"

namespace qat::ast {

class VectorType final : public Type {
	Type*               subType;
	PrerunExpression*   count;
	Maybe<FileRangePtr> scalable;

  public:
	VectorType(Type* _subType, PrerunExpression* _count, Maybe<FileRangePtr> _scalable, FileRangePtr _fileRange)
	    : Type(_fileRange), subType(_subType), count(_count), scalable(_scalable) {}

	static VectorType* create(Type* _subType, PrerunExpression* _count, Maybe<FileRangePtr> _scalable,
	                          FileRangePtr _fileRange) {
		return std::construct_at(OwnNormal(VectorType), _subType, _count, _scalable, _fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
	                         EmitCtx* ctx) final;

	Maybe<usize> get_type_bitsize(EmitCtx* ctx) const final;

	ir::Type* emit(EmitCtx* ctx);

	AstTypeKind type_kind() const final { return AstTypeKind::VECTOR; }

	String to_string() const final;
};

} // namespace qat::ast

#endif
