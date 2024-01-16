#ifndef QAT_AST_TYPES_VOID_HPP
#define QAT_AST_TYPES_VOID_HPP

#include "./qat_type.hpp"

namespace qat::ast {

class VoidType : public QatType {
public:
  explicit VoidType(FileRange _fileRange) : QatType(_fileRange) {}

  useit static inline VoidType* create(FileRange _fileRange) {
    return std::construct_at(OwnNormal(VoidType), _fileRange);
  }

  Maybe<usize> getTypeSizeInBits(IR::Context* ctx) const final;

  useit IR::QatType* emit(IR::Context* ctx);
  useit AstTypeKind  typeKind() const;
  useit Json         toJson() const;
  useit String       toString() const;
};

} // namespace qat::ast

#endif