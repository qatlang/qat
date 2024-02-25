#ifndef QAT_AST_TYPES_FLOAT_HPP
#define QAT_AST_TYPES_FLOAT_HPP

#include "./qat_type.hpp"

namespace qat::ast {

class FloatType final : public QatType {
private:
  IR::FloatTypeKind kind;

public:
  FloatType(IR::FloatTypeKind _kind, FileRange _fileRange) : QatType(_fileRange), kind(_kind) {}

  useit static inline FloatType* create(IR::FloatTypeKind _kind, FileRange _fileRange) {
    return std::construct_at(OwnNormal(FloatType), _kind, _fileRange);
  }

  useit static String kindToString(IR::FloatTypeKind kind);
  useit Maybe<usize> getTypeSizeInBits(IR::Context* ctx) const final;
  useit IR::QatType* emit(IR::Context* ctx);
  useit AstTypeKind  typeKind() const;
  useit Json         toJson() const;
  useit String       toString() const;
};

} // namespace qat::ast

#endif