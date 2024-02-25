#include "./not.hpp"

namespace qat::ast {

IR::Value* LogicalNot::emit(IR::Context* ctx) {
  auto* expEmit = exp->emit(ctx);
  auto* expTy   = expEmit->getType();
  if (expTy->isReference()) {
    expTy = expTy->asReference()->getSubType();
  }
  if (expTy->isBool()) {
    if (expEmit->isImplicitPointer() || expEmit->isReference()) {
      expEmit->loadImplicitPointer(ctx->builder);
      if (expEmit->isReference()) {
        expEmit = new IR::Value(ctx->builder.CreateLoad(expTy->getLLVMType(), expEmit->getLLVM()), expTy, false,
                                IR::Nature::temporary);
      }
    }
    return new IR::Value(ctx->builder.CreateNot(expEmit->getLLVM()), expTy, false, IR::Nature::temporary);
  } else {
    ctx->Error("Invalid expression for the not operator. The expression provided is of type " +
                   ctx->highlightError(expTy->toString()),
               fileRange);
  }
  return nullptr;
}

Json LogicalNot::toJson() const {
  return Json()._("nodeType", "notExpression")._("expression", exp->toJson())._("fileRange", fileRange);
}

} // namespace qat::ast