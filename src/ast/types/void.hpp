#ifndef QAT_AST_TYPES_VOID_HPP
#define QAT_AST_TYPES_VOID_HPP

#include "./qat_type.hpp"

namespace qat::ast {

class VoidType final : public Type {
  public:
	explicit VoidType(FileRangePtr _fileRange) : Type(_fileRange) {}

	static VoidType* create(FileRangePtr _fileRange) { return std::construct_at(OwnNormal(VoidType), _fileRange); }

	ir::Type* emit(EmitCtx* ctx);

	AstTypeKind type_kind() const;

	String to_string() const;
};

} // namespace qat::ast

#endif
