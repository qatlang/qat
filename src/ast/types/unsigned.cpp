#include "./unsigned.hpp"
#include "../../IR/types/unsigned.hpp"

namespace qat::ast {

UnsignedType::UnsignedType(u64 _bitWidth, bool _isBool, FileRange _fileRange)
    : QatType(std::move(_fileRange)), bitWidth(_bitWidth), isBool(_isBool) {}

Maybe<usize> UnsignedType::getTypeSizeInBits(IR::Context* ctx) const {
  return (usize)(ctx->getMod()->getLLVMModule()->getDataLayout().getTypeAllocSizeInBits(
      llvm::Type::getIntNTy(ctx->llctx, bitWidth)));
}

IR::QatType* UnsignedType::emit(IR::Context* ctx) {
  if (ctx->getMod()->hasUnsignedBitwidth(bitWidth)) {
    if (isBool) {
      return IR::UnsignedType::getBool(ctx->llctx);
    } else {
      return IR::UnsignedType::get(bitWidth, ctx->llctx);
    }
  } else {
    ctx->Error("This unsigned integer bitwidth is not allowed to be used since it is not brought into the module scope",
               fileRange);
  }
  return nullptr;
}

bool UnsignedType::isBitWidth(const u32 width) const { return bitWidth == width; }

TypeKind UnsignedType::typeKind() const { return TypeKind::unsignedInteger; }

Json UnsignedType::toJson() const {
  return Json()._("typeKind", "unsignedInteger")._("isBool", isBool)._("bitWidth", bitWidth)._("fileRange", fileRange);
}

String UnsignedType::toString() const { return isBool ? "bool" : ("u" + std::to_string(bitWidth)); }

} // namespace qat::ast