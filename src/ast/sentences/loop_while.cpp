#include "./loop_while.hpp"
#include "../../IR/control_flow.hpp"

namespace qat::ast {

LoopWhile::LoopWhile(Expression* _condition, Vec<Sentence*> _sentences, Maybe<Identifier> _tag, FileRange _fileRange)
    : Sentence(std::move(_fileRange)), condition(_condition), sentences(std::move(_sentences)), tag(std::move(_tag)) {}

IR::Value* LoopWhile::emit(IR::Context* ctx) {
  String uniq;
  if (tag.has_value()) {
    uniq = tag->value;
    for (const auto& info : ctx->loopsInfo) {
      if (info->name == uniq) {
        ctx->Error("The tag provided for this loop is already used by another loop", fileRange);
      }
    }
    for (const auto& brek : ctx->breakables) {
      if (brek->tag.has_value() && (brek->tag.value() == tag->value)) {
        ctx->Error("The tag provided for the loop is already used by another loop or switch", tag->range);
      }
    }
  } else {
    uniq = utils::unique_id();
  }
  auto* cond = condition->emit(ctx);
  cond->loadImplicitPointer(ctx->builder);
  if (cond->getType()->isUnsignedInteger() ||
      (cond->getType()->isReference() && cond->getType()->asReference()->getSubType()->isUnsignedInteger())) {
    auto* fun       = ctx->fn;
    auto* trueBlock = new IR::Block(fun, fun->getBlock());
    auto* condBlock = new IR::Block(fun, fun->getBlock());
    auto* restBlock = new IR::Block(fun, nullptr);
    auto* llCond    = cond->getLLVM();
    if (cond->getType()->isReference()) {
      llCond = ctx->builder.CreateLoad(cond->getType()->asReference()->getSubType()->getLLVMType(), llCond);
    }
    ctx->builder.CreateCondBr(llCond, trueBlock->getBB(), restBlock->getBB());
    ctx->loopsInfo.push_back(new IR::LoopInfo(uniq, trueBlock, condBlock, restBlock, nullptr, IR::LoopType::While));
    ctx->breakables.push_back(new IR::Breakable(tag.has_value() ? Maybe<String>(uniq) : None, restBlock, trueBlock));
    trueBlock->setActive(ctx->builder);
    emitSentences(sentences, ctx);
    trueBlock->destroyLocals(ctx);
    ctx->loopsInfo.pop_back();
    ctx->breakables.pop_back();
    (void)IR::addBranch(ctx->builder, condBlock->getBB());
    condBlock->setActive(ctx->builder);
    cond = condition->emit(ctx);
    cond->loadImplicitPointer(ctx->builder);
    if (cond->getType()->isReference()) {
      llCond = ctx->builder.CreateLoad(cond->getType()->asReference()->getSubType()->getLLVMType(), cond->getLLVM());
    } else {
      llCond = cond->getLLVM();
    }
    ctx->builder.CreateCondBr(llCond, trueBlock->getBB(), restBlock->getBB());
    restBlock->setActive(ctx->builder);
  } else {
    ctx->Error("The type of expression for the condition is not of unsigned "
               "integer type",
               condition->fileRange);
  }
  return nullptr;
}

Json LoopWhile::toJson() const {
  Vec<JsonValue> snts;
  for (auto* snt : sentences) {
    snts.push_back(snt->toJson());
  }
  return Json()
      ._("nodeType", "loopWhile")
      ._("condition", condition->toJson())
      ._("sentences", snts)
      ._("fileRange", fileRange);
}

} // namespace qat::ast