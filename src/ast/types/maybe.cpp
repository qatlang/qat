#include "./maybe.hpp"
#include "../../IR/types/maybe.hpp"
#include "qat_type.hpp"

namespace qat::ast {

MaybeType::MaybeType(bool _isVariable, QatType* _subType, FileRange _fileRange)
    : QatType(_isVariable, std::move(_fileRange)), subTyp(_subType) {}

IR::QatType* MaybeType::emit(IR::Context* ctx) {
  auto* subType = subTyp->emit(ctx);
  if (subType->isVoid()) {
    ctx->Error(ctx->highlightError("maybe void") + " is not a valid type", fileRange);
  }
  return IR::MaybeType::get(subType, ctx->llctx);
}

Json MaybeType::toJson() const {
  return Json()._("typeKind", "maybe")._("subType", subTyp->toJson())._("fileRange", fileRange);
}

String MaybeType::toString() const { return String(isVariable() ? "var " : "") + "maybe " + subTyp->toString(); }

} // namespace qat::ast