#include "./loop_infinite.hpp"
#include "../../IR/control_flow.hpp"

namespace qat::ast {

LoopInfinite::LoopInfinite(Vec<Sentence*> _sentences, Maybe<String> _tag, FileRange _fileRange)
    : Sentence(std::move(_fileRange)), sentences(std::move(_sentences)), tag(std::move(_tag)) {}

IR::Value* LoopInfinite::emit(IR::Context* ctx) {
  String uniqueName;
  if (tag.has_value()) {
    uniqueName = tag.value();
    for (const auto& info : ctx->loopsInfo) {
      if (info.name == uniqueName) {
        ctx->Error("The tag provided for this loop is already used by another loop", fileRange);
      }
    }
    for (const auto& brek : ctx->breakables) {
      if (brek.tag.has_value() && (brek.tag.value() == tag.value())) {
        ctx->Error("The tag provided for the loop is already used by another "
                   "loop or switch",
                   fileRange);
      }
    }
  } else {
    uniqueName = utils::unique_id();
  }
  auto* trueBlock = new IR::Block(ctx->fn, ctx->fn->getBlock());
  SHOW("Infinite loop true block " << trueBlock->getName())
  auto* restBlock = new IR::Block(ctx->fn, nullptr);
  restBlock->linkPrevBlock(ctx->fn->getBlock());
  SHOW("Infinite loop rest block " << restBlock->getName())
  (void)IR::addBranch(ctx->builder, trueBlock->getBB());
  trueBlock->setActive(ctx->builder);
  ctx->loopsInfo.push_back(IR::LoopInfo(uniqueName, trueBlock, nullptr, restBlock, nullptr, IR::LoopType::infinite));
  ctx->breakables.push_back(IR::Breakable(tag.has_value() ? Maybe<String>(uniqueName) : None, restBlock, trueBlock));
  emitSentences(sentences, ctx);
  trueBlock->destroyLocals(ctx);
  ctx->loopsInfo.pop_back();
  ctx->breakables.pop_back();
  (void)IR::addBranch(ctx->builder, trueBlock->getBB());
  restBlock->setActive(ctx->builder);
  return nullptr;
}

Json LoopInfinite::toJson() const {
  Vec<JsonValue> snts;
  for (auto* snt : sentences) {
    snts.push_back(snt->toJson());
  }
  return Json()
      ._("nodeType", "loopNormal")
      ._("sentences", snts)
      ._("hasTag", tag.has_value())
      ._("tag", tag.value_or(""))
      ._("fileRange", fileRange);
}

} // namespace qat::ast