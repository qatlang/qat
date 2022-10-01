#include "./not.hpp"

namespace qat::ast {

Not::Not(Expression* _exp, utils::FileRange range) : Expression(std::move(range)), exp(_exp) {}

IR::Value* Not::emit(IR::Context* ctx) {
  auto* expEmit = exp->emit(ctx);
  auto* expTy   = expEmit->getType();
  if (expTy->isReference()) {
    expTy = expTy->asReference()->getSubType();
  }
  if (expTy->isUnsignedInteger() && (expTy->asUnsignedInteger()->getBitwidth() == 1u)) {
    if (expEmit->isImplicitPointer() || expEmit->isReference()) {
      expEmit = new IR::Value(ctx->builder.CreateLoad(expTy->getLLVMType(), expEmit->getLLVM()), expTy, false,
                              IR::Nature::temporary);
    }
    return new IR::Value(ctx->builder.CreateNot(expEmit->getLLVM()), expTy, false, IR::Nature::temporary);
  } else {
    ctx->Error("Invalid expression for the not operator", fileRange);
  }
  return nullptr;
}

Json Not::toJson() const {
  return Json()._("nodeType", "notExpression")._("expression", exp->toJson())._("fileRange", fileRange);
}

} // namespace qat::ast