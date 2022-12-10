#include "./reference.hpp"
#include "../../IR/types/reference.hpp"

namespace qat::ast {

ReferenceType::ReferenceType(QatType* _type, bool _variable, FileRange _fileRange)
    : QatType(_variable, std::move(_fileRange)), type(_type) {}

IR::QatType* ReferenceType::emit(IR::Context* ctx) {
  auto* typEmit = type->emit(ctx);
  if (typEmit->isReference()) {
    ctx->Error("Subtype of reference cannot be a reference", fileRange);
  } else if (typEmit->isVoid()) {
    ctx->Error("Subtype of reference cannot be void", fileRange);
  }
  return IR::ReferenceType::get(type->isVariable(), typEmit, ctx->llctx);
}

TypeKind ReferenceType::typeKind() const { return TypeKind::reference; }

Json ReferenceType::toJson() const {
  return Json()
      ._("typeKind", "reference")
      ._("subType", type->toJson())
      ._("isVariable", isVariable())
      ._("fileRange", fileRange);
}

String ReferenceType::toString() const { return (isVariable() ? "var @" : "@") + type->toString(); }

} // namespace qat::ast