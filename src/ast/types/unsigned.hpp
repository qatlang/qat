#ifndef QAT_AST_TYPES_UNSIGNED_HPP
#define QAT_AST_TYPES_UNSIGNED_HPP

#include "../../IR/context.hpp"
#include "qat_type.hpp"

namespace qat::ast {

class UnsignedType : public QatType {
private:
  u32  bitWidth;
  bool isBool;

public:
  UnsignedType(u64 _bitWidth, bool _variable, bool _isBool, FileRange _fileRange);

  useit Maybe<usize> getTypeSizeInBits(IR::Context* ctx) const final;

  useit IR::QatType* emit(IR::Context* ctx);
  useit TypeKind     typeKind() const final;
  useit bool         isBitWidth(u32 width) const;
  useit Json         toJson() const final;
  useit String       toString() const final;
};

} // namespace qat::ast

#endif