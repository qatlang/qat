#include "./move.hpp"
#include "../../IR/logic.hpp"

namespace qat::ast {

Move::Move(Expression *_exp, utils::FileRange _fileRange)
    : Expression(std::move(_fileRange)), exp(_exp) {}

IR::Value *Move::emit(IR::Context *ctx) {
  auto *expEmit = exp->emit(ctx);
  if ((expEmit->getType()->isCoreType() && expEmit->isImplicitPointer() &&
       expEmit->isVariable()) ||
      (expEmit->getType()->isReference() &&
       expEmit->getType()->asReference()->getSubType()->isCoreType() &&
       expEmit->getType()->asReference()->isSubtypeVariable())) {
    auto *cTy = expEmit->isReference()
                    ? expEmit->getType()->asReference()->getSubType()->asCore()
                    : expEmit->getType()->asCore();
    if (cTy->hasMoveConstructor()) {
      auto *mvFn = cTy->getMoveConstructor();
      auto *newVal =
          IR::Logic::newAlloca(ctx->fn, utils::unique_id(), cTy->getLLVMType());
      (void)mvFn->call(ctx, {newVal, expEmit->getLLVM()}, ctx->getMod());
      return new IR::Value(newVal, cTy, false, IR::Nature::temporary);
    } else {
      ctx->Error("Core type " + ctx->highlightError(cTy->getFullName()) +
                     " does not have a move ",
                 fileRange);
    }
  } else {
    ctx->Error("Invalid value provided to move expression. The value provided "
               "should either be a variable reference or a local variable",
               fileRange);
  }
  return nullptr;
}

nuo::Json Move::toJson() const {
  return nuo::Json()
      ._("nodeType", "move")
      ._("expression", exp->toJson())
      ._("fileRange", fileRange);
}

} // namespace qat::ast