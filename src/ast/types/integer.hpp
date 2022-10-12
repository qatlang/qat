#ifndef QAT_AST_TYPES_INTEGER_HPP
#define QAT_AST_TYPES_INTEGER_HPP

#include "../../IR/context.hpp"
#include "qat_type.hpp"

namespace qat::ast {

class IntegerType : public QatType {
private:
  const u32 bitWidth;

public:
  IntegerType(u32 _bitWidth, bool _variable, utils::FileRange _fileRange);

  IR::QatType* emit(IR::Context* ctx);
  TypeKind     typeKind() const;
  bool         isBitWidth(const u32 width) const;
  Json         toJson() const;
  String       toString() const;
};

} // namespace qat::ast

#endif