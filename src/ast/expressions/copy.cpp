#include "./copy.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/maybe.hpp"

namespace qat::ast {

Copy::Copy(Expression* _exp, utils::FileRange _fileRange) : Expression(std::move(_fileRange)), exp(_exp) {}

IR::Value* Copy::emit(IR::Context* ctx) {
  auto* expEmit = exp->emit(ctx);
  auto* expTy   = expEmit->getType();
  if ((expTy->isCoreType() && expEmit->isImplicitPointer()) ||
      (expTy->isReference() && expTy->asReference()->getSubType()->isCoreType())) {
    auto* cTy = expEmit->isReference() ? expEmit->getType()->asReference()->getSubType()->asCore()
                                       : expEmit->getType()->asCore();
    if (cTy->hasCopyConstructor()) {
      llvm::Value* alloca = nullptr;
      if (local) {
        if (!cTy->isSame(local->getType()) &&
            (local->getType()->isMaybe() && !cTy->isSame(local->getType()->asMaybe()->getSubType()))) {
          ctx->Error("The type provided in the local declaration does not match the type to be copied", fileRange);
        }
        alloca = local->getType()->isMaybe()
                     ? ctx->builder.CreateStructGEP(local->getType()->getLLVMType(), local->getAlloca(), 1u)
                     : local->getAlloca();
      } else if (irName.has_value()) {
        local  = ctx->fn->getBlock()->newValue(irName.value(), cTy, isVar);
        alloca = local->getAlloca();
      } else {
        alloca = IR::Logic::newAlloca(ctx->fn, utils::unique_id(), cTy->getLLVMType());
      }
      (void)cTy->getCopyConstructor()->call(ctx, {alloca, expEmit->getLLVM()}, ctx->getMod());
      if (local) {
        if (local->getType()->isMaybe()) {
          ctx->builder.CreateStore(
              llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u),
              ctx->builder.CreateStructGEP(local->getType()->getLLVMType(), local->getAlloca(), 0u));
        }
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
    ctx->Error(
        "The value provided should either be a reference or a local variable. The provided value does not reside in an address and hence cannot be copied from",
        fileRange);
  }
  return nullptr;
}

Json Copy::toJson() const {
  return Json()._("nodeType", "copyExpression")._("expression", exp->toJson())._("fileRange", fileRange);
}

} // namespace qat::ast