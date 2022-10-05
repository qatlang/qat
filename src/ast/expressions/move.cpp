#include "./move.hpp"
#include "../../IR/logic.hpp"

namespace qat::ast {

Move::Move(Expression* _exp, utils::FileRange _fileRange) : Expression(std::move(_fileRange)), exp(_exp) {}

IR::Value* Move::emit(IR::Context* ctx) {
  auto* expEmit = exp->emit(ctx);
  if ((expEmit->getType()->isCoreType() && expEmit->isImplicitPointer() && expEmit->isVariable()) ||
      (expEmit->getType()->isReference() && expEmit->getType()->asReference()->getSubType()->isCoreType() &&
       expEmit->getType()->asReference()->isSubtypeVariable())) {
    auto* cTy = expEmit->isReference() ? expEmit->getType()->asReference()->getSubType()->asCore()
                                       : expEmit->getType()->asCore();
    if (cTy->hasMoveConstructor()) {
      llvm::AllocaInst* alloca = nullptr;
      if (local) {
        if (!cTy->isSame(local->getType())) {
          ctx->Error("The type provided in the local declaration does not match the type to be moved", fileRange);
        }
        alloca = local->getAlloca();
      } else if (irName.has_value()) {
        SHOW("Created local for move from name")
        local  = ctx->fn->getBlock()->newValue(irName.value(), cTy, isVar);
        alloca = local->getAlloca();
      } else {
        alloca = IR::Logic::newAlloca(ctx->fn, utils::unique_id(), cTy->getLLVMType());
      }
      (void)(cTy->getMoveConstructor()->call(ctx, {alloca, expEmit->getLLVM()}, ctx->getMod()));
      if (local) {
        auto* val = new IR::Value(local->getAlloca(), local->getType(), local->isVariable(), local->getNature());
        val->setLocalID(local->getLocalID());
        return val;
      } else {
        return new IR::Value(alloca, cTy, true, IR::Nature::pure);
      }
    } else {
      ctx->Error("Core type " + ctx->highlightError(cTy->getFullName()) + " does not have a move ", fileRange);
    }
  } else {
    ctx->Error("Invalid value provided to move expression. The value provided "
               "should either be a variable reference or a local variable",
               fileRange);
  }
  return nullptr;
}

Json Move::toJson() const {
  return Json()._("nodeType", "move")._("expression", exp->toJson())._("fileRange", fileRange);
}

} // namespace qat::ast