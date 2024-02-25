#include "./integer.hpp"
#include "../../IR/types/integer.hpp"
#include <string>

namespace qat::ast {

Maybe<usize> IntegerType::getTypeSizeInBits(IR::Context* ctx) const {
  return (usize)(ctx->getMod()->getLLVMModule()->getDataLayout().getTypeAllocSizeInBits(
      llvm::Type::getIntNTy(ctx->llctx, bitWidth)));
}

IR::QatType* IntegerType::emit(IR::Context* ctx) {
  SHOW("IntegerType::emit")
  if (bitWidth > 128) {
    ctx->Error("Arbitrary integer bitwidths above 128 are not allowed at the moment", fileRange);
  }
  SHOW("Curr mod is " << ctx->getMod())
  if (isPartOfGeneric || ctx->getMod()->hasIntegerBitwidth(bitWidth)) {
    return IR::IntegerType::get(bitWidth, ctx);
  } else {
    ctx->Error("The signed integer bitwidth " + ctx->highlightError(std::to_string(bitWidth)) +
                   " is not allowed to be used since it is not brought into into the module " +
                   ctx->highlightError(ctx->getMod()->getName()) + " in file " + ctx->getMod()->getFilePath(),
               fileRange);
  }
  return nullptr;
}

bool IntegerType::isBitWidth(const u32 width) const { return bitWidth == width; }

AstTypeKind IntegerType::typeKind() const { return AstTypeKind::INTEGER; }

Json IntegerType::toJson() const {
  return Json()._("typeKind", "integer")._("bitWidth", bitWidth)._("fileRange", fileRange);
}

String IntegerType::toString() const { return "i" + std::to_string(bitWidth); }

} // namespace qat::ast