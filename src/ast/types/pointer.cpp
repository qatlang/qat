#include "./pointer.hpp"
#include "../../IR/types/pointer.hpp"
#include "../../show.hpp"

namespace qat::ast {

PointerType::PointerType(QatType *_type, const bool _variable,
                         const utils::FileRange _fileRange)
    : type(_type), QatType(_variable, _fileRange) {}

IR::QatType *PointerType::emit(IR::Context *ctx) {
  return IR::PointerType::get(type->isVariable(), type->emit(ctx), ctx->llctx);
}

TypeKind PointerType::typeKind() const { return TypeKind::pointer; }

nuo::Json PointerType::toJson() const {
  return nuo::Json()
      ._("typeKind", "pointer")
      ._("subType", type->toJson())
      ._("isVariable", isVariable())
      ._("fileRange", fileRange);
}

String PointerType::toString() const {
  return (isVariable() ? "var #[" : "#[") + type->toString() + "]";
}

} // namespace qat::ast