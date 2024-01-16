#ifndef QAT_AST_TYPES_STRING_SLICE_HPP
#define QAT_AST_TYPES_STRING_SLICE_HPP

#include "../../IR/context.hpp"
#include "./qat_type.hpp"

namespace qat::ast {

class StringSliceType : public QatType {
public:
  explicit StringSliceType(FileRange _fileRange) : QatType(_fileRange) {}

  useit static inline StringSliceType* create(FileRange _fileRange) {
    return std::construct_at(OwnNormal(StringSliceType), _fileRange);
  }

  useit Maybe<usize> getTypeSizeInBits(IR::Context* ctx) const final;
  useit IR::QatType* emit(IR::Context* ctx) final;
  useit AstTypeKind  typeKind() const final;
  useit Json         toJson() const final;
  useit String       toString() const final;
};

} // namespace qat::ast

#endif