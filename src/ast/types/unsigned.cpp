#include "./unsigned.hpp"
#include "../../IR/types/unsigned.hpp"

namespace qat::ast {

UnsignedType::UnsignedType(u64 _bitWidth, bool _variable, utils::FileRange _fileRange)
    : QatType(_variable, std::move(_fileRange)), bitWidth(_bitWidth) {}

IR::QatType* UnsignedType::emit(IR::Context* ctx) {
  if (ctx->getMod()->hasUnsignedBitwidth(bitWidth)) {
    return IR::UnsignedType::get(bitWidth, ctx->llctx);
  } else {
    ctx->Error("This unsigned integer bitwidth is not allowed to be used since it is not brought into the module scope",
               fileRange);
  }
  return nullptr;
}

bool UnsignedType::isBitWidth(const u32 width) const { return bitWidth == width; }

TypeKind UnsignedType::typeKind() const { return TypeKind::unsignedInteger; }

Json UnsignedType::toJson() const {
  return Json()
      ._("typeKind", "unsignedInteger")
      ._("bitWidth", bitWidth)
      ._("isVariable", isVariable())
      ._("fileRange", fileRange);
}

String UnsignedType::toString() const { return (isVariable() ? "var u" : "u") + std::to_string(bitWidth); }

} // namespace qat::ast