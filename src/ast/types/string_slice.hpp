#ifndef QAT_AST_TYPES_STRING_SLICE_HPP
#define QAT_AST_TYPES_STRING_SLICE_HPP

#include "../../IR/context.hpp"
#include "./qat_type.hpp"

namespace qat::ast {

class StringSliceType final : public Type {
  public:
	explicit StringSliceType(FileRange _fileRange) : Type(_fileRange) {}

	useit static StringSliceType* create(FileRange _fileRange) {
		return std::construct_at(OwnNormal(StringSliceType), _fileRange);
	}

	useit Maybe<usize> get_type_bitsize(EmitCtx* ctx) const final;
	useit ir::Type*   emit(EmitCtx* ctx) final;
	useit AstTypeKind type_kind() const final;
	useit Json        to_json() const final;
	useit String      to_string() const final;
};

} // namespace qat::ast

#endif
