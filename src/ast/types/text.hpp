#ifndef QAT_AST_TYPES_TEXT_HPP
#define QAT_AST_TYPES_TEXT_HPP

#include "./qat_type.hpp"

namespace qat::ast {

class TextType final : public Type {
  public:
	explicit TextType(FileRangePtr _fileRange) : Type(_fileRange) {}

	static TextType* create(FileRangePtr _fileRange) { return std::construct_at(OwnNormal(TextType), _fileRange); }

	Maybe<usize> get_type_bitsize(EmitCtx* ctx) const final;

	ir::Type* emit(EmitCtx* ctx) final;

	AstTypeKind type_kind() const final;

	String to_string() const final;
};

} // namespace qat::ast

#endif
