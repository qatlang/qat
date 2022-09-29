#include "./integer.hpp"
#include "../../IR/types/integer.hpp"
#include <string>

namespace qat::ast {

IntegerType::IntegerType(const u32 _bitWidth, const bool _variable,
                         const utils::FileRange _fileRange)
    : bitWidth(_bitWidth), QatType(_variable, _fileRange) {}

IR::QatType *IntegerType::emit(IR::Context *ctx) {
  return IR::IntegerType::get(bitWidth, ctx->llctx);
}

bool IntegerType::isBitWidth(const u32 width) const {
  return bitWidth == width;
}

TypeKind IntegerType::typeKind() const { return TypeKind::integer; }

Json IntegerType::toJson() const {
  return Json()
      ._("typeKind", "integer")
      ._("bitWidth", bitWidth)
      ._("isVariable", isVariable())
      ._("fileRange", fileRange);
}

String IntegerType::toString() const {
  return (isVariable() ? "var i" : "i") + std::to_string(bitWidth);
}

} // namespace qat::ast