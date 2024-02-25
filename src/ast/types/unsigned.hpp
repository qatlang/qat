#ifndef QAT_AST_TYPES_UNSIGNED_HPP
#define QAT_AST_TYPES_UNSIGNED_HPP

#include "../../IR/context.hpp"
#include "qat_type.hpp"

namespace qat::ast {

class UnsignedType final : public QatType {
  friend class BringBitwidths;
  friend class FillGeneric;

private:
  u32  bitWidth;
  bool isBool;

  mutable bool isPartOfGeneric = false;

public:
  UnsignedType(u64 _bitWidth, bool _isBool, FileRange _fileRange)
      : QatType(_fileRange), bitWidth(_bitWidth), isBool(_isBool) {}

  useit static inline UnsignedType* create(u64 _bitWidth, bool _isBool, FileRange _fileRange) {
    return std::construct_at(OwnNormal(UnsignedType), _bitWidth, _isBool, _fileRange);
  }

  useit Maybe<usize> getTypeSizeInBits(IR::Context* ctx) const final;

  useit IR::QatType* emit(IR::Context* ctx);
  useit AstTypeKind  typeKind() const final;
  useit bool         isBitWidth(u32 width) const;
  useit Json         toJson() const final;
  useit String       toString() const final;
};

} // namespace qat::ast

#endif