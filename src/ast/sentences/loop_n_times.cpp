#include "./loop_n_times.hpp"
#include "../../IR/control_flow.hpp"
#include "../../utils/unique_id.hpp"

namespace qat::ast {

LoopNTimes::LoopNTimes(Expression* _count, Vec<Sentence*> _snts, Maybe<Identifier> _tag, FileRange _fileRange)
    : Sentence(std::move(_fileRange)), sentences(std::move(_snts)), count(_count), tag(std::move(_tag)) {}

bool LoopNTimes::hasTag() const { return tag.has_value(); }

IR::Value* LoopNTimes::emit(IR::Context* ctx) {
  if (tag.has_value()) {
    for (const auto& info : ctx->loopsInfo) {
      if (info->name == tag.value().value) {
        ctx->Error("The tag provided for the loop is already used for another loop", fileRange);
      }
    }
    for (const auto& brek : ctx->breakables) {
      if (brek->tag.has_value() && (brek->tag.value() == tag.value().value)) {
        ctx->Error("The tag provided for the loop is already used by another "
                   "loop or switch",
                   fileRange);
      }
    }
  }
  auto* limit   = count->emit(ctx);
  auto* countTy = limit->getType();
  limit->loadImplicitPointer(ctx->builder);
  if (limit->getType()->isUnsignedInteger() ||
      (limit->getType()->isReference() && limit->getType()->asReference()->getSubType()->isUnsignedInteger()) ||
      limit->getType()->isInteger() ||
      (limit->getType()->isReference() && limit->getType()->asReference()->getSubType()->isInteger())) {
    auto* llCount = limit->getLLVM();
    if (limit->getType()->isReference()) {
      countTy = limit->getType()->asReference()->getSubType();
      llCount = ctx->builder.CreateLoad(countTy->getLLVMType(), llCount);
    }
    auto  uniq      = hasTag() ? tag.value().value : utils::unique_id();
    auto* loopIndex = ctx->fn->getBlock()->newValue("loop'index'" + utils::unique_id(), countTy, false,
                                                    tag.has_value() ? tag->range : fileRange);
    ctx->builder.CreateStore(llvm::ConstantInt::get(countTy->getLLVMType(), 0u), loopIndex->getAlloca());
    auto* trueBlock = new IR::Block(ctx->fn, ctx->fn->getBlock());
    SHOW("loop times true block " << ctx->fn->getFullName() << "." << trueBlock->getName())
    auto* condBlock = new IR::Block(ctx->fn, ctx->fn->getBlock());
    SHOW("loop times cond block " << ctx->fn->getFullName() << "." << condBlock->getName())
    auto* restBlock = new IR::Block(ctx->fn, nullptr);
    restBlock->linkPrevBlock(ctx->fn->getBlock());
    SHOW("loop times rest block " << ctx->fn->getFullName() << "." << restBlock->getName())
    ctx->builder.CreateCondBr(
        (countTy->isUnsignedInteger()
             ? ctx->builder.CreateICmpULT(
                   ctx->builder.CreateLoad(loopIndex->getType()->getLLVMType(), loopIndex->getAlloca()), llCount)
             : ctx->builder.CreateAnd(
                   ctx->builder.CreateICmpSGT(llCount, llvm::ConstantInt::get(limit->getType()->getLLVMType(), 0u)),
                   ctx->builder.CreateICmpSLT(
                       ctx->builder.CreateLoad(loopIndex->getType()->getLLVMType(), loopIndex->getAlloca()), llCount))),
        trueBlock->getBB(), restBlock->getBB());
    ctx->loopsInfo.push_back(new IR::LoopInfo(uniq, trueBlock, condBlock, restBlock, loopIndex, IR::LoopType::nTimes));
    ctx->breakables.push_back(new IR::Breakable(uniq, restBlock, trueBlock));
    trueBlock->setActive(ctx->builder);
    emitSentences(sentences, ctx);
    trueBlock->destroyLocals(ctx);
    (void)IR::addBranch(ctx->builder, condBlock->getBB());
    condBlock->setActive(ctx->builder);
    ctx->builder.CreateStore(
        ctx->builder.CreateAdd(ctx->builder.CreateLoad(loopIndex->getType()->getLLVMType(), loopIndex->getAlloca()),
                               llvm::ConstantInt::get(loopIndex->getType()->getLLVMType(), 1u)),
        loopIndex->getAlloca());
    ctx->builder.CreateCondBr(
        (countTy->isUnsignedInteger()
             ? ctx->builder.CreateICmpULT(
                   ctx->builder.CreateLoad(loopIndex->getType()->getLLVMType(), loopIndex->getAlloca()), llCount)
             : ctx->builder.CreateICmpSLT(
                   ctx->builder.CreateLoad(loopIndex->getType()->getLLVMType(), loopIndex->getAlloca()), llCount)),
        trueBlock->getBB(), restBlock->getBB());
    ctx->loopsInfo.pop_back();
    ctx->breakables.pop_back();
    restBlock->setActive(ctx->builder);
  } else {
    ctx->Error("The count for loop should be of signed or unsigned integer type", count->fileRange);
  }
  return nullptr;
}

Json LoopNTimes::toJson() const {
  Vec<JsonValue> snts;
  for (auto* snt : sentences) {
    snts.push_back(snt->toJson());
  }
  return Json()._("nodeType", "loopTimes")._("count", count->toJson())._("sentences", snts)._("fileRange", fileRange);
}

} // namespace qat::ast