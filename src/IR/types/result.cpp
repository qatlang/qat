#include "./result.hpp"
#include "../context.hpp"

namespace qat::IR {

ResultType::ResultType(IR::QatType* _resTy, IR::QatType* _errTy, bool _isPacked, IR::Context* ctx)
    : validType(_resTy), errorType(_errTy), isPacked(_isPacked) {
  const usize validTypeSize =
      validType->isTypeSized()
          ? ((validType->isOpaque() && !validType->asOpaque()->hasSubType())
                 ? validType->asOpaque()->getDeducedSize()
                 : ((usize)(ctx->dataLayout.value().getTypeAllocSizeInBits(validType->getLLVMType()))))
          : 8u;
  const usize errorTypeSize =
      errorType->isTypeSized()
          ? ((errorType->isOpaque() && !errorType->asOpaque()->hasSubType())
                 ? errorType->asOpaque()->getDeducedSize()
                 : ((usize)(ctx->dataLayout.value().getTypeAllocSizeInBits(errorType->getLLVMType()))))
          : 8u;
  linkingName = "qat'result:[" + String(isPacked ? "pack," : "") + validType->getNameForLinking() + "," +
                errorType->getNameForLinking() + "]";
  llvmType = llvm::StructType::create(
      ctx->llctx,
      {llvm::Type::getInt1Ty(ctx->llctx),
       llvm::Type::getIntNTy(ctx->llctx, (validTypeSize > errorTypeSize) ? validTypeSize : errorTypeSize)},
      linkingName, isPacked);
}

ResultType* ResultType::get(IR::QatType* validType, IR::QatType* errorType, bool isPacked, IR::Context* ctx) {
  for (auto typ : allQatTypes) {
    if (typ->isResult() && typ->asResult()->getValidType()->isSame(validType) &&
        typ->asResult()->getErrorType()->isSame(errorType) && (typ->asResult()->isPacked == isPacked)) {
      return typ->asResult();
    }
  }
  return new ResultType(validType, errorType, isPacked, ctx);
}

IR::QatType* ResultType::getValidType() const { return validType; }

IR::QatType* ResultType::getErrorType() const { return errorType; }

bool ResultType::isTypePacked() const { return isPacked; }

bool ResultType::isTypeSized() const { return true; }

TypeKind ResultType::typeKind() const { return TypeKind::result; }

String ResultType::toString() const { return "result:[" + validType->toString() + ", " + errorType->toString() + "]"; }

} // namespace qat::IR
