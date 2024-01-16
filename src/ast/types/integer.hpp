#ifndef QAT_AST_TYPES_INTEGER_HPP
#define QAT_AST_TYPES_INTEGER_HPP

#include "../../IR/context.hpp"
#include "qat_type.hpp"

namespace qat::ast {

class IntegerType : public QatType {
  friend class BringBitwidths;
  friend class FillGeneric;

private:
  const u32 bitWidth;

  mutable bool isPartOfGeneric = false;

public:
  IntegerType(u32 _bitWidth, FileRange _fileRange) : QatType(_fileRange), bitWidth(_bitWidth) {}

  useit static inline IntegerType* create(u32 _bitWidth, FileRange _fileRange) {
    return std::construct_at(OwnNormal(IntegerType), _bitWidth, _fileRange);
  }

  useit Maybe<usize> getTypeSizeInBits(IR::Context* ctx) const final;

  IR::QatType* emit(IR::Context* ctx);
  AstTypeKind  typeKind() const;
  bool         isBitWidth(const u32 width) const;
  Json         toJson() const;
  String       toString() const;
};

} // namespace qat::ast

#endif