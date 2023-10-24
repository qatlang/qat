#include "./maybe.hpp"
#include "../../IR/types/maybe.hpp"
#include "qat_type.hpp"

namespace qat::ast {

MaybeType::MaybeType(bool _isPacked, QatType* _subType, FileRange _fileRange)
    : QatType(std::move(_fileRange)), subTyp(_subType), isPacked(_isPacked) {}

Maybe<usize> MaybeType::getTypeSizeInBits(IR::Context* ctx) const {
  auto subTySize = subTyp->getTypeSizeInBits(ctx);
  if (subTySize.has_value()) {
    return (usize)(ctx->getMod()->getLLVMModule()->getDataLayout().getTypeAllocSizeInBits(llvm::StructType::create(
        {llvm::Type::getInt1Ty(ctx->llctx), llvm::Type::getIntNTy(ctx->llctx, subTySize.value())})));
  } else {
    return None;
  }
}

IR::QatType* MaybeType::emit(IR::Context* ctx) {
  auto* subType = subTyp->emit(ctx);
  if (subType->isOpaque() && !subType->asOpaque()->hasSubType()) {
    ctx->Error("The subtype of " + ctx->highlightError("maybe") +
                   " is an incomplete opaque type: " + ctx->highlightError(subType->toString()),
               fileRange);
  }
  // if (subType->isVoid()) {
  //   ctx->Error(ctx->highlightError("maybe void") + " is not a valid type", fileRange);
  // }
  return IR::MaybeType::get(subType, isPacked, ctx);
}

Json MaybeType::toJson() const {
  return Json()._("typeKind", "maybe")._("isPacked", isPacked)._("subType", subTyp->toJson())._("fileRange", fileRange);
}

String MaybeType::toString() const { return String(isPacked ? "pack " : "") + "maybe:[" + subTyp->toString() + "]"; }

} // namespace qat::ast