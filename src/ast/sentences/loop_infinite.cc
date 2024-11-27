#include "./loop_infinite.hpp"
#include "../../IR/control_flow.hpp"

namespace qat::ast {

ir::Value* LoopInfinite::emit(EmitCtx* ctx) {
  String uniqueName;
  if (tag.has_value()) {
    uniqueName = tag->value;
    for (const auto& info : ctx->loopsInfo) {
      if (info.name.has_value() && (info.name->value == uniqueName)) {
        ctx->Error("The tag provided for this loop is already used by another loop", tag->range,
                   Pair<String, FileRange>{"The existing tag was found here", info.name->range});
      }
      if (info.secondaryName.has_value() && (info.secondaryName->value == uniqueName)) {
        ctx->Error("The tag provided for this loop is already used by another loop", tag->range,
                   Pair<String, FileRange>{"The existing tag was found here", info.secondaryName->range});
      }
    }
    for (const auto& brek : ctx->breakables) {
      if (brek.tag.has_value() && (brek.tag->value == tag->value)) {
        ctx->Error("The tag provided for the loop is already used by another " + ctx->color(brek.type_to_string()),
                   tag->range, Pair<String, FileRange>{"The existing tag was found here", brek.tag->range});
      }
    }
    auto block = ctx->get_fn()->get_block();
    if (block->has_value(tag->value)) {
      ctx->Error("There already exists another local value with the same name as this tag", tag->range,
                 block->get_value(tag->value)->has_associated_range()
                     ? Maybe<Pair<String, FileRange>>(
                           {"The local value was found here", block->get_value(tag->value)->get_file_range()})
                     : None);
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
  ctx->loopsInfo.push_back(LoopInfo(tag, trueBlock, nullptr, restBlock, nullptr, LoopType::INFINITE));
  ctx->breakables.push_back(Breakable(BreakableType::loop, tag, restBlock, trueBlock));
  emit_sentences(sentences, ctx);
  trueBlock->destroy_locals(ctx);
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
