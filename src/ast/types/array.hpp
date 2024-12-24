#ifndef QAT_AST_TYPES_ARRAY_HPP
#define QAT_AST_TYPES_ARRAY_HPP

#include "./qat_type.hpp"

namespace qat::ast {

class PrerunExpression;

/**
 *  A continuous sequence of elements of a particular type. The sequence
 * is fixed length
 *
 */
class ArrayType final : public Type {
  private:
	Type*                     elementType;
	mutable PrerunExpression* lengthExp;

	void typeInferenceForLength(ir::Ctx* irCtx) const;

  public:
	ArrayType(Type* _element_type, PrerunExpression* _length, FileRange _fileRange)
	    : Type(_fileRange), elementType(_element_type), lengthExp(_length) {}

	useit static ArrayType* create(Type* _element_type, PrerunExpression* _length, FileRange _fileRange) {
		return std::construct_at(OwnNormal(ArrayType), _element_type, _length, _fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
	                         EmitCtx* ctx) final;

	useit Maybe<usize> get_type_bitsize(EmitCtx* ctx) const final;

	useit ir::Type* emit(EmitCtx* ctx) final;

	useit AstTypeKind type_kind() const;

	useit Json to_json() const;

	useit String to_string() const;
};

} // namespace qat::ast

#endif
