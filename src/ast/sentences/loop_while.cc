#include "./loop_while.hpp"
#include "../../IR/control_flow.hpp"

namespace qat::ast {

ir::Value* LoopWhile::emit(EmitCtx* ctx) {
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
  ir::Value* cond = isDoAndLoop ? nullptr : condition->emit(ctx);
  if (cond == nullptr || cond->get_ir_type()->is_bool() ||
      (cond->get_ir_type()->is_reference() && cond->get_ir_type()->as_reference()->get_subtype()->is_bool())) {
    auto*        fun       = ctx->get_fn();
    auto*        trueBlock = new ir::Block(fun, fun->get_block());
    auto*        condBlock = new ir::Block(fun, fun->get_block());
    auto*        restBlock = new ir::Block(fun, fun->get_block());
    llvm::Value* llCond    = nullptr;
    if (isDoAndLoop) {
      (void)ir::add_branch(ctx->irCtx->builder, trueBlock->get_bb());
    } else {
      llCond = cond->get_llvm();
      cond->load_ghost_pointer(ctx->irCtx->builder);
      if (cond->get_ir_type()->is_reference()) {
        cond->load_ghost_pointer(ctx->irCtx->builder);
        llCond = ctx->irCtx->builder.CreateLoad(cond->get_ir_type()->as_reference()->get_subtype()->get_llvm_type(),
                                                cond->get_llvm());
      }
      ctx->irCtx->builder.CreateCondBr(llCond, trueBlock->get_bb(), restBlock->get_bb());
    }
    ctx->loopsInfo.push_back(
        LoopInfo(uniq, trueBlock, condBlock, restBlock, nullptr, isDoAndLoop ? LoopType::doWhile : LoopType::While));
    ctx->breakables.push_back(Breakable(tag.has_value() ? Maybe<String>(uniq) : None, restBlock, trueBlock));
    trueBlock->set_active(ctx->irCtx->builder);
    emit_sentences(sentences, ctx);
    trueBlock->destroyLocals(ctx);
    ctx->loopsInfo.pop_back();
    ctx->breakables.pop_back();
    (void)ir::add_branch(ctx->irCtx->builder, condBlock->get_bb());
    condBlock->set_active(ctx->irCtx->builder);
    cond = condition->emit(ctx);
    if (isDoAndLoop) {
      if (!cond->get_ir_type()->is_bool() &&
          !(cond->get_ir_type()->is_reference() && cond->get_ir_type()->as_reference()->get_subtype()->is_bool())) {
        ctx->Error("The condition for the " + ctx->color("do-while") + " loop should be of " + ctx->color("bool") +
                       " type. Got an expression of type " + ctx->color(cond->get_ir_type()->to_string()) +
                       " instead. Please check if you forgot to add a comparison, or made a mistake in the expression",
                   condition->fileRange);
      }
    }
    cond->load_ghost_pointer(ctx->irCtx->builder);
    if (cond->get_ir_type()->is_reference()) {
      llCond = ctx->irCtx->builder.CreateLoad(cond->get_ir_type()->as_reference()->get_subtype()->get_llvm_type(),
                                              cond->get_llvm());
    } else {
      llCond = cond->get_llvm();
    }
    ctx->irCtx->builder.CreateCondBr(llCond, trueBlock->get_bb(), restBlock->get_bb());
    restBlock->set_active(ctx->irCtx->builder);
  } else {
    ctx->Error("The expression used for the condition of " + ctx->color(isDoAndLoop ? "do-while" : "while") +
                   " loop should be of " + ctx->color("bool") + " type. Got an expression of type " +
                   ctx->color(cond->get_ir_type()->to_string()) +
                   " instead.Please check if you forgot to add a comparison, or made a mistake in the expression",
               condition->fileRange);
  }
  return nullptr;
}

Json LoopWhile::to_json() const {
  Vec<JsonValue> snts;
  for (auto* snt : sentences) {
    snts.push_back(snt->to_json());
  }
  return Json()
      ._("nodeType", "loopWhile")
      ._("condition", condition->to_json())
      ._("sentences", snts)
      ._("fileRange", fileRange);
}

} // namespace qat::ast