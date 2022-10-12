#include "./integer.hpp"
#include "../../IR/types/integer.hpp"
#include <string>

namespace qat::ast {

IntegerType::IntegerType(u32 _bitWidth, bool _variable, utils::FileRange _fileRange)
    : QatType(_variable, std::move(_fileRange)), bitWidth(_bitWidth) {}

IR::QatType* IntegerType::emit(IR::Context* ctx) {
  if (ctx->getMod()->hasIntegerBitwidth(bitWidth)) {
    return IR::IntegerType::get(bitWidth, ctx->llctx);
  } else {
    ctx->Error("This signed integer bitwidth is not allowed to be used since it is not brought into the module scope",
               fileRange);
  }
  return nullptr;
}

bool IntegerType::isBitWidth(const u32 width) const { return bitWidth == width; }

TypeKind IntegerType::typeKind() const { return TypeKind::integer; }

Json IntegerType::toJson() const {
  return Json()
      ._("typeKind", "integer")
      ._("bitWidth", bitWidth)
      ._("isVariable", isVariable())
      ._("fileRange", fileRange);
}

String IntegerType::toString() const { return (isVariable() ? "var i" : "i") + std::to_string(bitWidth); }

} // namespace qat::ast