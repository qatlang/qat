#include "./bitwise_not.hpp"

namespace qat::ast {

IR::PrerunValue* PrerunBitwiseNot::emit(IR::Context* ctx) {
  auto val   = value->emit(ctx);
  auto valTy = val->getType();
  if (valTy->isCType()) {
    valTy = valTy->asCType()->getSubType();
  }
  if (valTy->isInteger() || valTy->isUnsignedInteger()) {
    return new IR::PrerunValue(llvm::ConstantExpr::getNot(val->getLLVMConstant()), val->getType());
  }
  ctx->Error("The value is of type " + ctx->highlightError(val->getType()->toString()) +
                 " and hence does not support this operator",
             fileRange);
  return nullptr;
}

String PrerunBitwiseNot::toString() const { return "~" + value->toString(); }

Json PrerunBitwiseNot::toJson() const {
  return Json()._("nodeType", "prerunBitwiseNot")._("value", value->toJson())._("fileRange", fileRange);
}

} // namespace qat::ast