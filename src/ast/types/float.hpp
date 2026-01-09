#ifndef QAT_AST_TYPES_FLOAT_HPP
#define QAT_AST_TYPES_FLOAT_HPP

#include "./qat_type.hpp"

namespace qat::ast {

class FloatType final : public Type {
  private:
	ir::FloatTypeKind kind;

  public:
	FloatType(ir::FloatTypeKind _kind, FileRangePtr _fileRange) : Type(_fileRange), kind(_kind) {}

	static FloatType* create(ir::FloatTypeKind _kind, FileRangePtr _fileRange) {
		return std::construct_at(OwnNormal(FloatType), _kind, _fileRange);
	}

	static String kindToString(ir::FloatTypeKind kind);

	Maybe<usize> get_type_bitsize(EmitCtx* ctx) const final;

	ir::Type* emit(EmitCtx* ctx);

	AstTypeKind type_kind() const;

	String to_string() const;
};

} // namespace qat::ast

#endif
