#ifndef QAT_AST_TYPES_ARRAY_HPP
#define QAT_AST_TYPES_ARRAY_HPP

#include "./qat_type.hpp"

namespace qat::ast {

class PrerunExpression;

class ArrayType final : public Type {
  private:
	Type*                     elementType;
	mutable PrerunExpression* lengthExp;

	void typeInferenceForLength(ir::Ctx* irCtx) const;

  public:
	ArrayType(Type* _element_type, PrerunExpression* _length, FileRangePtr _fileRange)
	    : Type(_fileRange), elementType(_element_type), lengthExp(_length) {}

	static ArrayType* create(Type* _element_type, PrerunExpression* _length, FileRangePtr _fileRange) {
		return std::construct_at(OwnNormal(ArrayType), _element_type, _length, _fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
	                         EmitCtx* ctx) final;

	Maybe<usize> get_type_bitsize(EmitCtx* ctx) const final;

	ir::Type* emit(EmitCtx* ctx) final;

	AstTypeKind type_kind() const;

	String to_string() const;
};

} // namespace qat::ast

#endif
