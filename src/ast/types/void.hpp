#ifndef QAT_AST_TYPES_VOID_HPP
#define QAT_AST_TYPES_VOID_HPP

#include "./qat_type.hpp"

namespace qat::ast {

class VoidType final : public Type {
  public:
	explicit VoidType(FileRange _fileRange) : Type(_fileRange) {}

	useit static VoidType* create(FileRange _fileRange) { return std::construct_at(OwnNormal(VoidType), _fileRange); }

	useit ir::Type*   emit(EmitCtx* ctx);
	useit AstTypeKind type_kind() const;
	useit Json        to_json() const;
	useit String      to_string() const;
};

} // namespace qat::ast

#endif
