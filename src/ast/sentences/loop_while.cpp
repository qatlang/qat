#include "./loop_while.hpp"

namespace qat::ast {

LoopWhile::LoopWhile(Expression *_condition, Vec<Sentence *> _sentences,
                     utils::FileRange _fileRange)
    : Sentence(std::move(_fileRange)), condition(_condition),
      sentences(std::move(_sentences)) {}

IR::Value *LoopWhile::emit(IR::Context *ctx) {
  auto *cond = condition->emit(ctx);
  cond->loadImplicitPointer(ctx->builder);
  if (cond->getType()->isUnsignedInteger() ||
      (cond->getType()->isReference() &&
       cond->getType()->asReference()->getSubType()->isUnsignedInteger())) {
    auto *fun       = ctx->fn;
    auto *trueBlock = new IR::Block(fun, fun->getBlock());
    auto *restBlock = new IR::Block(fun, fun->getBlock());
    auto *llCond    = cond->getLLVM();
    if (cond->getType()->isReference()) {
      llCond = ctx->builder.CreateLoad(
          cond->getType()->asReference()->getSubType()->getLLVMType(), llCond);
    }
    ctx->builder.CreateCondBr(llCond, trueBlock->getBB(), restBlock->getBB());
    ctx->breakables.push_back(new IR::Breakable(None, restBlock));
    trueBlock->setActive(ctx->builder);
    for (auto *snt : sentences) {
      (void)snt->emit(ctx);
    }
    cond = condition->emit(ctx);
    cond->loadImplicitPointer(ctx->builder);
    if (cond->getType()->isReference()) {
      llCond = ctx->builder.CreateLoad(
          cond->getType()->asReference()->getSubType()->getLLVMType(),
          cond->getLLVM());
    } else {
      llCond = cond->getLLVM();
    }
    ctx->builder.CreateCondBr(llCond, trueBlock->getBB(), restBlock->getBB());
    ctx->breakables.pop_back();
    restBlock->setActive(ctx->builder);
  } else {
    ctx->Error("The type of expression for the condition is not of unsigned "
               "integer type",
               condition->fileRange);
  }
  return nullptr;
}

nuo::Json LoopWhile::toJson() const {
  Vec<nuo::JsonValue> snts;
  for (auto *snt : sentences) {
    snts.push_back(snt->toJson());
  }
  return nuo::Json()
      ._("nodeType", "loopWhile")
      ._("condition", condition->toJson())
      ._("sentences", snts)
      ._("fileRange", fileRange);
}

} // namespace qat::ast