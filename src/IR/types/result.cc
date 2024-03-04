#include "./result.hpp"
#include "../context.hpp"

namespace qat::ir {

ResultType::ResultType(ir::Type* _resTy, ir::Type* _errTy, bool _isPacked, ir::Ctx* irCtx)
    : validType(_resTy), errorType(_errTy), isPacked(_isPacked) {
  const usize validTypeSize =
      validType->is_type_sized()
          ? ((validType->is_opaque() && !validType->as_opaque()->has_subtype())
                 ? validType->as_opaque()->get_deduced_size()
                 : ((usize)(irCtx->dataLayout.value().getTypeAllocSizeInBits(validType->get_llvm_type()))))
          : 8u;
  const usize errorTypeSize =
      errorType->is_type_sized()
          ? ((errorType->is_opaque() && !errorType->as_opaque()->has_subtype())
                 ? errorType->as_opaque()->get_deduced_size()
                 : ((usize)(irCtx->dataLayout.value().getTypeAllocSizeInBits(errorType->get_llvm_type()))))
          : 8u;
  linkingName = "qat'result:[" + String(isPacked ? "pack," : "") + validType->get_name_for_linking() + "," +
                errorType->get_name_for_linking() + "]";
  llvmType = llvm::StructType::create(
      irCtx->llctx,
      {llvm::Type::getInt1Ty(irCtx->llctx),
       llvm::Type::getIntNTy(irCtx->llctx, (validTypeSize > errorTypeSize) ? validTypeSize : errorTypeSize)},
      linkingName, isPacked);
}

ResultType* ResultType::get(ir::Type* validType, ir::Type* errorType, bool isPacked, ir::Ctx* irCtx) {
  for (auto typ : allTypes) {
    if (typ->is_result() && typ->as_result()->getValidType()->is_same(validType) &&
        typ->as_result()->getErrorType()->is_same(errorType) && (typ->as_result()->isPacked == isPacked)) {
      return typ->as_result();
    }
  }
  return new ResultType(validType, errorType, isPacked, irCtx);
}

ir::Type* ResultType::getValidType() const { return validType; }

ir::Type* ResultType::getErrorType() const { return errorType; }

bool ResultType::isTypePacked() const { return isPacked; }

bool ResultType::is_type_sized() const { return true; }

TypeKind ResultType::type_kind() const { return TypeKind::result; }

String ResultType::to_string() const {
  return "result:[" + validType->to_string() + ", " + errorType->to_string() + "]";
}

} // namespace qat::ir
