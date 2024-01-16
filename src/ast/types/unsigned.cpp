#include "./unsigned.hpp"
#include "../../IR/types/unsigned.hpp"

namespace qat::ast {

Maybe<usize> UnsignedType::getTypeSizeInBits(IR::Context* ctx) const {
  return (usize)(ctx->getMod()->getLLVMModule()->getDataLayout().getTypeAllocSizeInBits(
      llvm::Type::getIntNTy(ctx->llctx, bitWidth)));
}

IR::QatType* UnsignedType::emit(IR::Context* ctx) {
  if (bitWidth > 128) {
    ctx->Error("Arbitrary unsigned integer bitwidths above 128 are not allowed at the moment", fileRange);
  }
  if (isBool) {
    return IR::UnsignedType::getBool(ctx);
  } else if (isPartOfGeneric || ctx->getMod()->hasUnsignedBitwidth(bitWidth)) {
    return IR::UnsignedType::get(bitWidth, ctx);
  } else {
    ctx->Error("The unsigned integer bitwidth " + ctx->highlightError(std::to_string(bitWidth)) +
                   " is not allowed to be used since it is not brought into the module " +
                   ctx->highlightError(ctx->getMod()->getName()) + " in file " + ctx->getMod()->getFilePath(),
               fileRange);
  }
  return nullptr;
}

bool UnsignedType::isBitWidth(const u32 width) const { return bitWidth == width; }

AstTypeKind UnsignedType::typeKind() const { return AstTypeKind::UNSIGNED_INTEGER; }

Json UnsignedType::toJson() const {
  return Json()._("typeKind", "unsignedInteger")._("isBool", isBool)._("bitWidth", bitWidth)._("fileRange", fileRange);
}

String UnsignedType::toString() const { return isBool ? "bool" : ("u" + std::to_string(bitWidth)); }

} // namespace qat::ast