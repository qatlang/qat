#include "./result.hpp"
#include "../../IR/types/result.hpp"
#include "qat_type.hpp"

namespace qat::ast {

ResultType::ResultType(ast::QatType* _validType, ast::QatType* _errorType, bool _isPacked, FileRange _fileRange)
    : QatType(_fileRange), validType(_validType), errorType(_errorType), isPacked(_isPacked) {}

IR::QatType* ResultType::emit(IR::Context* ctx) {
  auto* validRes = validType->emit(ctx);
  if (validRes->isOpaque() && !validRes->asOpaque()->hasSubType() && !validRes->asOpaque()->hasDeducedSize()) {
    ctx->Error("The valid value type " + ctx->highlightError(validRes->toString()) +
                   " provided is an incomplete type with unknown size",
               validType->fileRange);
  } else if (!validRes->isTypeSized()) {
    ctx->Error("The valid value type " + ctx->highlightError(validRes->toString()) + " should be a sized type",
               validType->fileRange);
  }
  auto* errorRes = errorType->emit(ctx);
  if (errorRes->isOpaque() && !errorRes->asOpaque()->hasSubType() && !errorRes->asOpaque()->hasDeducedSize()) {
    ctx->Error("The error value type " + ctx->highlightError(errorRes->toString()) +
                   " provided is an incomplete type with unknown size",
               errorType->fileRange);
  } else if (!errorRes->isTypeSized()) {
    ctx->Error("The error value type " + ctx->highlightError(errorRes->toString()) + " should be a sized type",
               errorType->fileRange);
  }
  return IR::ResultType::get(validRes, errorRes, isPacked, ctx);
}

TypeKind ResultType::typeKind() const { return TypeKind::result; }

String ResultType::toString() const {
  return "result:[" + String(isPacked ? "pack, " : "") + validType->toString() + ", " + errorType->toString() + "]";
}

Json ResultType::toJson() const {
  return Json()
      ._("typeKind", "result")
      ._("validType", validType->toJson())
      ._("errorType", errorType->toJson())
      ._("isPacked", isPacked)
      ._("fileRange", fileRange);
}

} // namespace qat::ast