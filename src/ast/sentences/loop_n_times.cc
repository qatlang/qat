#include "./loop_n_times.hpp"
#include "../../IR/control_flow.hpp"
#include "../../utils/unique_id.hpp"

namespace qat::ast {

bool LoopNTimes::hasTag() const { return tag.has_value(); }

ir::Value* LoopNTimes::emit(EmitCtx* ctx) {
  if (tag.has_value()) {
    for (const auto& info : ctx->loopsInfo) {
      if (info.name == tag.value().value) {
        ctx->Error("The tag provided for the loop is already used for another loop", fileRange);
      }
    }
    for (const auto& brek : ctx->breakables) {
      if (brek.tag.has_value() && (brek.tag.value() == tag.value().value)) {
        ctx->Error("The tag provided for the loop is already used by another "
                   "loop or switch",
                   fileRange);
      }
    }
    if (ctx->get_fn()->get_block()->has_value(tag->value)) {
      ctx->Error("There already exists another local value with the same name as this tag", tag->range);
    }
  }
  auto* limit = count->emit(ctx);
  auto* countTy =
      limit->get_ir_type()->is_reference() ? limit->get_ir_type()->as_reference()->get_subtype() : limit->get_ir_type();
  auto* originalLimitTy = countTy;
  limit->load_ghost_pointer(ctx->irCtx->builder);
  if (countTy->is_unsigned_integer() || countTy->is_integer() ||
      (countTy->is_ctype() && (countTy->as_ctype()->get_subtype()->is_integer() ||
                               countTy->as_ctype()->get_subtype()->is_unsigned_integer()))) {
    if (countTy->is_ctype()) {
      countTy = countTy->as_ctype()->get_subtype();
    }
    auto* llCount = limit->get_llvm();
    if (limit->get_ir_type()->is_reference()) {
      llCount = ctx->irCtx->builder.CreateLoad(countTy->get_llvm_type(), llCount);
    }
    auto  uniq = hasTag() ? tag.value().value : utils::unique_id();
    auto* loopIndex =
        ctx->get_fn()->get_block()->new_value(uniq, originalLimitTy, false, tag.has_value() ? tag->range : fileRange);
    ctx->irCtx->builder.CreateStore(llvm::ConstantInt::get(countTy->get_llvm_type(), 0u), loopIndex->get_alloca());
    auto* loopBlock = new ir::Block(ctx->get_fn(), ctx->get_fn()->get_block());
    auto* trueBlock = new ir::Block(ctx->get_fn(), loopBlock);
    SHOW("loop times true block " << ctx->get_fn()->get_full_name() << "." << trueBlock->get_name())
    auto* condBlock = new ir::Block(ctx->get_fn(), loopBlock);
    SHOW("loop times cond block " << ctx->get_fn()->get_full_name() << "." << condBlock->get_name())
    auto* restBlock = new ir::Block(ctx->get_fn(), loopBlock->get_parent()->get_parent());
    restBlock->link_previous_block(loopBlock->get_parent());
    SHOW("loop times rest block " << ctx->get_fn()->get_full_name() << "." << restBlock->get_name())
    (void)ir::add_branch(ctx->irCtx->builder, loopBlock->get_bb());
    loopBlock->set_active(ctx->irCtx->builder);
    ctx->irCtx->builder.CreateCondBr(
        (countTy->is_unsigned_integer()
             ? ctx->irCtx->builder.CreateICmpULT(
                   ctx->irCtx->builder.CreateLoad(loopIndex->get_ir_type()->get_llvm_type(), loopIndex->get_alloca()),
                   llCount)
             : ctx->irCtx->builder.CreateAnd(
                   ctx->irCtx->builder.CreateICmpSGT(llCount,
                                                     llvm::ConstantInt::get(limit->get_ir_type()->get_llvm_type(), 0u)),
                   ctx->irCtx->builder.CreateICmpSLT(
                       ctx->irCtx->builder.CreateLoad(loopIndex->get_ir_type()->get_llvm_type(),
                                                      loopIndex->get_alloca()),
                       llCount))),
        trueBlock->get_bb(), restBlock->get_bb());
    ctx->loopsInfo.push_back(LoopInfo(uniq, trueBlock, condBlock, restBlock, loopIndex, LoopType::nTimes));
    ctx->breakables.push_back(Breakable(uniq, restBlock, trueBlock));
    trueBlock->set_active(ctx->irCtx->builder);
    emit_sentences(sentences, ctx);
    trueBlock->destroy_locals(ctx);
    (void)ir::add_branch(ctx->irCtx->builder, condBlock->get_bb());
    condBlock->set_active(ctx->irCtx->builder);
    ctx->irCtx->builder.CreateStore(
        ctx->irCtx->builder.CreateAdd(
            ctx->irCtx->builder.CreateLoad(loopIndex->get_ir_type()->get_llvm_type(), loopIndex->get_alloca()),
            llvm::ConstantInt::get(loopIndex->get_ir_type()->get_llvm_type(), 1u)),
        loopIndex->get_alloca());
    ctx->irCtx->builder.CreateCondBr(
        (countTy->is_unsigned_integer()
             ? ctx->irCtx->builder.CreateICmpULT(
                   ctx->irCtx->builder.CreateLoad(loopIndex->get_ir_type()->get_llvm_type(), loopIndex->get_alloca()),
                   llCount)
             : ctx->irCtx->builder.CreateICmpSLT(
                   ctx->irCtx->builder.CreateLoad(loopIndex->get_ir_type()->get_llvm_type(), loopIndex->get_alloca()),
                   llCount)),
        trueBlock->get_bb(), restBlock->get_bb());
    ctx->loopsInfo.pop_back();
    ctx->breakables.pop_back();
    restBlock->set_active(ctx->irCtx->builder);
  } else {
    ctx->Error("The count for loop should be of signed or unsigned integer type", count->fileRange);
  }
  return nullptr;
}

Json LoopNTimes::to_json() const {
  Vec<JsonValue> snts;
  for (auto* snt : sentences) {
    snts.push_back(snt->to_json());
  }
  return Json()._("nodeType", "loopTimes")._("count", count->to_json())._("sentences", snts)._("fileRange", fileRange);
}

} // namespace qat::ast
