#include "./copy.hpp"
#include "../../IR/logic.hpp"

namespace qat::ast {

Copy::Copy(Expression* _exp, utils::FileRange _fileRange) : Expression(std::move(_fileRange)), exp(_exp) {}

IR::Value* Copy::emit(IR::Context* ctx) {
  auto* expEmit = exp->emit(ctx);
  if ((expEmit->getType()->isCoreType() && expEmit->isImplicitPointer()) ||
      (expEmit->getType()->isReference() && expEmit->getType()->asReference()->getSubType()->isCoreType())) {
    auto* cTy = expEmit->isReference() ? expEmit->getType()->asReference()->getSubType()->asCore()
                                       : expEmit->getType()->asCore();
    if (cTy->hasCopyConstructor()) {
      auto* cpFn   = cTy->getCopyConstructor();
      auto* newVal = IR::Logic::newAlloca(ctx->fn, utils::unique_id(), cTy->getLLVMType());
      (void)cpFn->call(ctx, {newVal, expEmit->getLLVM()}, ctx->getMod());
      return new IR::Value(newVal, cTy, false, IR::Nature::temporary);
    } else {
      ctx->Error("Core type " + ctx->highlightError(cTy->getFullName()) + " does not have a copy constructor",
                 fileRange);
    }
  } else {
    ctx->Error("Invalid value provided to copy expression. The value provided "
               "should either be a reference or a local variable",
               fileRange);
  }
  return nullptr;
}

Json Copy::toJson() const {
  return Json()._("nodeType", "copyExpression")._("expression", exp->toJson())._("fileRange", fileRange);
}

} // namespace qat::ast