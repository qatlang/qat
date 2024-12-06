#ifndef QAT_AST_TYPES_FLOAT_HPP
#define QAT_AST_TYPES_FLOAT_HPP

#include "./qat_type.hpp"

namespace qat::ast {

class FloatType final : public Type {
private:
  ir::FloatTypeKind kind;

public:
  FloatType(ir::FloatTypeKind _kind, FileRange _fileRange) : Type(_fileRange), kind(_kind) {}

  useit static FloatType* create(ir::FloatTypeKind _kind, FileRange _fileRange) {
    return std::construct_at(OwnNormal(FloatType), _kind, _fileRange);
  }

  useit static String kindToString(ir::FloatTypeKind kind);
  useit Maybe<usize> getTypeSizeInBits(EmitCtx* ctx) const final;
  useit ir::Type*   emit(EmitCtx* ctx);
  useit AstTypeKind type_kind() const;
  useit Json        to_json() const;
  useit String      to_string() const;
};

} // namespace qat::ast

#endif
