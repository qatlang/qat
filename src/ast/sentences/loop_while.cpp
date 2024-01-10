#include "./loop_while.hpp"
#include "../../IR/control_flow.hpp"

namespace qat::ast {

LoopWhile::LoopWhile(bool _isDoAndLoop, Expression* _condition, Vec<Sentence*> _sentences, Maybe<Identifier> _tag,
                     FileRange _fileRange)
    : Sentence(std::move(_fileRange)), condition(_condition), sentences(std::move(_sentences)), tag(std::move(_tag)),
      isDoAndLoop(_isDoAndLoop) {}

IR::Value* LoopWhile::emit(IR::Context* ctx) {
  String uniq;
  if (tag.has_value()) {
    uniq = tag->value;
    for (const auto& info : ctx->loopsInfo) {
      if (info.name == uniq) {
        ctx->Error("The tag provided for this loop is already used by another loop", fileRange);
      }
    }
    for (const auto& brek : ctx->breakables) {
      if (brek.tag.has_value() && (brek.tag.value() == tag->value)) {
        ctx->Error("The tag provided for the loop is already used by another loop or switch", tag->range);
      }
    }
  } else {
    uniq = utils::unique_id();
  }
  IR::Value* cond = isDoAndLoop ? nullptr : condition->emit(ctx);
  if (cond == nullptr || cond->getType()->isBool() ||
      (cond->getType()->isReference() && cond->getType()->asReference()->getSubType()->isBool())) {
    auto*        fun       = ctx->getActiveFunction();
    auto*        trueBlock = new IR::Block(fun, fun->getBlock());
    auto*        condBlock = new IR::Block(fun, fun->getBlock());
    auto*        restBlock = new IR::Block(fun, fun->getBlock());
    llvm::Value* llCond    = nullptr;
    if (isDoAndLoop) {
      (void)IR::addBranch(ctx->builder, trueBlock->getBB());
    } else {
      llCond = cond->getLLVM();
      cond->loadImplicitPointer(ctx->builder);
      if (cond->getType()->isReference()) {
        cond->loadImplicitPointer(ctx->builder);
        llCond = ctx->builder.CreateLoad(cond->getType()->asReference()->getSubType()->getLLVMType(), cond->getLLVM());
      }
      ctx->builder.CreateCondBr(llCond, trueBlock->getBB(), restBlock->getBB());
    }
    ctx->loopsInfo.push_back(IR::LoopInfo(uniq, trueBlock, condBlock, restBlock, nullptr,
                                          isDoAndLoop ? IR::LoopType::doWhile : IR::LoopType::While));
    ctx->breakables.push_back(IR::Breakable(tag.has_value() ? Maybe<String>(uniq) : None, restBlock, trueBlock));
    trueBlock->setActive(ctx->builder);
    emitSentences(sentences, ctx);
    trueBlock->destroyLocals(ctx);
    ctx->loopsInfo.pop_back();
    ctx->breakables.pop_back();
    (void)IR::addBranch(ctx->builder, condBlock->getBB());
    condBlock->setActive(ctx->builder);
    cond = condition->emit(ctx);
    if (isDoAndLoop) {
      if (!cond->getType()->isBool() &&
          !(cond->getType()->isReference() && cond->getType()->asReference()->getSubType()->isBool())) {
        ctx->Error("The condition for the " + ctx->highlightError("do-while") + " loop should be of " +
                       ctx->highlightError("bool") + " type. Got an expression of type " +
                       ctx->highlightError(cond->getType()->toString()) +
                       " instead. Please check if you forgot to add a comparison, or made a mistake in the expression",
                   condition->fileRange);
      }
    }
    cond->loadImplicitPointer(ctx->builder);
    if (cond->getType()->isReference()) {
      llCond = ctx->builder.CreateLoad(cond->getType()->asReference()->getSubType()->getLLVMType(), cond->getLLVM());
    } else {
      llCond = cond->getLLVM();
    }
    ctx->builder.CreateCondBr(llCond, trueBlock->getBB(), restBlock->getBB());
    restBlock->setActive(ctx->builder);
  } else {
    ctx->Error("The expression used for the condition of " + ctx->highlightError(isDoAndLoop ? "do-while" : "while") +
                   " loop should be of " + ctx->highlightError("bool") + " type. Got an expression of type " +
                   ctx->highlightError(cond->getType()->toString()) +
                   " instead.Please check if you forgot to add a comparison, or made a mistake in the expression",
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