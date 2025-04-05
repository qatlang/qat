#ifndef QAT_AST_TYPES_TEXT_HPP
#define QAT_AST_TYPES_TEXT_HPP

#include "../../IR/context.hpp"
#include "./qat_type.hpp"

namespace qat::ast {

class TextType final : public Type {
  public:
	explicit TextType(FileRange _fileRange) : Type(_fileRange) {}

	useit static TextType* create(FileRange _fileRange) { return std::construct_at(OwnNormal(TextType), _fileRange); }

	useit Maybe<usize> get_type_bitsize(EmitCtx* ctx) const final;
	useit ir::Type*   emit(EmitCtx* ctx) final;
	useit AstTypeKind type_kind() const final;
	useit Json        to_json() const final;
	useit String      to_string() const final;
};

} // namespace qat::ast

#endif
