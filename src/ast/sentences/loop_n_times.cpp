#include "./loop_n_times.hpp"
#include "../../utils/unique_id.hpp"

namespace qat::ast {

LoopNTimes::LoopNTimes(Expression *_count, Vec<Sentence *> _snts,
                       String _indexName, utils::FileRange _fileRange)
    : Sentence(std::move(_fileRange)), sentences(std::move(_snts)),
      count(_count), indexName(std::move(_indexName)) {}

bool LoopNTimes::hasName() const { return !indexName.empty(); }

IR::Value *LoopNTimes::emit(IR::Context *ctx) {
  auto *limit   = count->emit(ctx);
  auto *countTy = limit->getType();
  limit->loadImplicitPointer(ctx->builder);
  if (limit->getType()->isUnsignedInteger() ||
      (limit->getType()->isReference() &&
       limit->getType()->asReference()->getSubType()->isUnsignedInteger()) ||
      limit->getType()->isInteger() ||
      (limit->getType()->isReference() &&
       limit->getType()->asReference()->getSubType()->isInteger())) {
    auto *llCount = limit->getLLVM();
    if (limit->getType()->isReference()) {
      countTy = limit->getType()->asReference()->getSubType();
      llCount = ctx->builder.CreateLoad(countTy->getLLVMType(), llCount);
    }
    auto  uniq = hasName() ? indexName : utils::unique_id();
    auto *loopIndex =
        ctx->fn->getBlock()->newValue("loop'index'" + uniq, countTy, false);
    ctx->builder.CreateStore(llvm::ConstantInt::get(countTy->getLLVMType(), 0u),
                             loopIndex->getAlloca());
    auto *trueBlock = new IR::Block(ctx->fn, ctx->fn->getBlock());
    auto *restBlock = new IR::Block(ctx->fn, ctx->fn->getBlock());
    ctx->builder.CreateCondBr(
        (countTy->isUnsignedInteger()
             ? ctx->builder.CreateICmpULT(
                   ctx->builder.CreateLoad(loopIndex->getType()->getLLVMType(),
                                           loopIndex->getAlloca()),
                   llCount)
             : ctx->builder.CreateAnd(
                   ctx->builder.CreateICmpSGT(
                       llCount, llvm::ConstantInt::get(
                                    limit->getType()->getLLVMType(), 0u)),
                   ctx->builder.CreateICmpSLT(
                       ctx->builder.CreateLoad(
                           loopIndex->getType()->getLLVMType(),
                           loopIndex->getAlloca()),
                       llCount))),
        trueBlock->getBB(), restBlock->getBB());
    ctx->loopsInfo.push_back(new IR::LoopInfo(uniq, trueBlock, restBlock,
                                              loopIndex, IR::LoopType::times));
    ctx->breakables.push_back(new IR::Breakable(uniq, restBlock));
    trueBlock->setActive(ctx->builder);
    for (auto *snt : sentences) {
      (void)snt->emit(ctx);
    }
    ctx->builder.CreateStore(
        ctx->builder.CreateAdd(
            ctx->builder.CreateLoad(loopIndex->getType()->getLLVMType(),
                                    loopIndex->getAlloca()),
            llvm::ConstantInt::get(loopIndex->getType()->getLLVMType(), 1u)),
        loopIndex->getAlloca());
    ctx->builder.CreateCondBr(
        (countTy->isUnsignedInteger()
             ? ctx->builder.CreateICmpULT(
                   ctx->builder.CreateLoad(loopIndex->getType()->getLLVMType(),
                                           loopIndex->getAlloca()),
                   llCount)
             : ctx->builder.CreateICmpSLT(
                   ctx->builder.CreateLoad(loopIndex->getType()->getLLVMType(),
                                           loopIndex->getAlloca()),
                   llCount)),
        trueBlock->getBB(), restBlock->getBB());
    ctx->loopsInfo.pop_back();
    ctx->breakables.pop_back();
    restBlock->setActive(ctx->builder);
  } else {
    ctx->Error(
        "The count for loop should be of integer or unsigned integer type",
        count->fileRange);
  }
  return nullptr;
}

nuo::Json LoopNTimes::toJson() const {
  Vec<nuo::JsonValue> snts;
  for (auto *snt : sentences) {
    snts.push_back(snt->toJson());
  }
  return nuo::Json()
      ._("nodeType", "loopTimes")
      ._("count", count->toJson())
      ._("sentences", snts)
      ._("fileRange", fileRange);
}

} // namespace qat::ast