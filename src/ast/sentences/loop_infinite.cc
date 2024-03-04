#include "./loop_infinite.hpp"
#include "../../IR/control_flow.hpp"

namespace qat::ast {

ir::Value* LoopInfinite::emit(EmitCtx* ctx) {
  String uniqueName;
  if (tag.has_value()) {
    uniqueName = tag.value().value;
    for (const auto& info : ctx->loopsInfo) {
      if (info.name == uniqueName) {
        ctx->Error("The tag provided for this loop is already used by another loop", fileRange);
      }
    }
    for (const auto& brek : ctx->breakables) {
      if (brek.tag.has_value() && (brek.tag.value() == tag.value().value)) {
        ctx->Error("The tag provided for the loop is already used by another loop or switch", fileRange);
      }
    }
  } else {
    uniqueName = utils::unique_id();
  }
  auto* trueBlock = new ir::Block(ctx->get_fn(), ctx->get_fn()->get_block());
  SHOW("Infinite loop true block " << trueBlock->get_name())
  auto* restBlock = new ir::Block(ctx->get_fn(), nullptr);
  restBlock->link_previous_block(ctx->get_fn()->get_block());
  SHOW("Infinite loop rest block " << restBlock->get_name())
  (void)ir::add_branch(ctx->irCtx->builder, trueBlock->get_bb());
  trueBlock->set_active(ctx->irCtx->builder);
  ctx->loopsInfo.push_back(LoopInfo(uniqueName, trueBlock, nullptr, restBlock, nullptr, LoopType::infinite));
  ctx->breakables.push_back(Breakable(tag.has_value() ? Maybe<String>(uniqueName) : None, restBlock, trueBlock));
  emit_sentences(sentences, ctx);
  trueBlock->destroyLocals(ctx);
  ctx->loopsInfo.pop_back();
  ctx->breakables.pop_back();
  (void)ir::add_branch(ctx->irCtx->builder, trueBlock->get_bb());
  restBlock->set_active(ctx->irCtx->builder);
  return nullptr;
}

Json LoopInfinite::to_json() const {
  Vec<JsonValue> snts;
  for (auto* snt : sentences) {
    snts.push_back(snt->to_json());
  }
  return Json()
      ._("nodeType", "loopNormal")
      ._("sentences", snts)
      ._("hasTag", tag.has_value())
      ._("tag", tag.has_value() ? tag.value() : JsonValue())
      ._("fileRange", fileRange);
}

} // namespace qat::ast