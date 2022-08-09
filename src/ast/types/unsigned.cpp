#include "./unsigned.hpp"
#include "../../IR/types/unsigned.hpp"

namespace qat::ast {

UnsignedType::UnsignedType(u64 _bitWidth, bool _variable,
                           utils::FileRange _fileRange)
    : QatType(_variable, std::move(_fileRange)), bitWidth(_bitWidth) {}

IR::QatType *UnsignedType::emit(IR::Context *ctx) {
  return IR::UnsignedType::get(bitWidth, ctx->llctx);
}

bool UnsignedType::isBitWidth(const u32 width) const {
  return bitWidth == width;
}

TypeKind UnsignedType::typeKind() const { return TypeKind::unsignedInteger; }

nuo::Json UnsignedType::toJson() const {
  return nuo::Json()
      ._("typeKind", "unsignedInteger")
      ._("bitWidth", bitWidth)
      ._("isVariable", isVariable())
      ._("fileRange", fileRange);
}

String UnsignedType::toString() const {
  return (isVariable() ? "var u" : "u") + std::to_string(bitWidth);
}

} // namespace qat::ast