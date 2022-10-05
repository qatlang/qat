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
      llvm::AllocaInst* alloca = nullptr;
      if (local) {
        if (!cTy->isSame(local->getType())) {
          ctx->Error("The type provided in the local declaration does not match the type to be copied", fileRange);
        }
        alloca = local->getAlloca();
      } else if (irName.has_value()) {
        local  = ctx->fn->getBlock()->newValue(irName.value(), cTy, isVar);
        alloca = local->getAlloca();
      } else {
        alloca = IR::Logic::newAlloca(ctx->fn, utils::unique_id(), cTy->getLLVMType());
      }
      (void)cTy->getCopyConstructor()->call(ctx, {alloca, expEmit->getLLVM()}, ctx->getMod());
      if (local) {
        auto* val = new IR::Value(local->getAlloca(), local->getType(), local->isVariable(), local->getNature());
        val->setLocalID(local->getLocalID());
        return val;
      } else {
        return new IR::Value(alloca, cTy, true, IR::Nature::pure);
      }
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