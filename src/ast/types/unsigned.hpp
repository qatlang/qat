#ifndef QAT_AST_TYPES_UNSIGNED_HPP
#define QAT_AST_TYPES_UNSIGNED_HPP

#include "../../IR/context.hpp"
#include "qat_type.hpp"

namespace qat::ast {

class UnsignedType : public QatType {
private:
  u32 bitWidth;

public:
  UnsignedType(u64 _bitWidth, bool _variable, FileRange _fileRange);

  IR::QatType* emit(IR::Context* ctx);
  TypeKind     typeKind() const final;
  bool         isBitWidth(const u32 width) const;
  Json         toJson() const;
  String       toString() const;
};

} // namespace qat::ast

#endif