#include "./continue.hpp"
#include "../../IR/control_flow.hpp"
#include "../../IR/types/void.hpp"

namespace qat::ast {

Continue::Continue(Maybe<Identifier> _tag, FileRange _fileRange)
    : Sentence(std::move(_fileRange)), tag(std::move(_tag)) {}

IR::Value* Continue::emit(IR::Context* ctx) {
  if (ctx->loopsInfo.empty()) {
    ctx->Error("Continue sentence is not present inside any loop block", fileRange);
  } else {
    if (tag.has_value()) {
      for (usize i = (ctx->loopsInfo.size() - 1); i >= 0; i--) {
        if (tag.value().value == ctx->loopsInfo.at(i).name) {
          if (ctx->loopsInfo.at(i).type == IR::LoopType::infinite) {
            IR::destroyLocalsFrom(ctx, ctx->loopsInfo.at(i).mainBlock);
            return new IR::Value(IR::addBranch(ctx->builder, ctx->loopsInfo.at(i).mainBlock->getBB()),
                                 IR::VoidType::get(ctx->llctx), false, IR::Nature::pure);
          } else {
            IR::destroyLocalsFrom(ctx, ctx->loopsInfo.at(i).mainBlock);
            return new IR::Value(IR::addBranch(ctx->builder, ctx->loopsInfo.at(i).condBlock->getBB()),
                                 IR::VoidType::get(ctx->llctx), false, IR::Nature::pure);
          }
          return nullptr;
        }
      }
      ctx->Error("The provided tag " + ctx->highlightError(tag->value) +
                     " does not match the tag of any parent loops or switches",
                 fileRange);
    } else {
      if (ctx->loopsInfo.size() == 1) {
        if (ctx->loopsInfo.front().type == IR::LoopType::infinite) {
          IR::destroyLocalsFrom(ctx, ctx->loopsInfo.front().mainBlock);
          return new IR::Value(IR::addBranch(ctx->builder, ctx->loopsInfo.front().mainBlock->getBB()),
                               IR::VoidType::get(ctx->llctx), false, IR::Nature::pure);
        } else {
          IR::destroyLocalsFrom(ctx, ctx->loopsInfo.front().mainBlock);
          return new IR::Value(IR::addBranch(ctx->builder, ctx->loopsInfo.front().condBlock->getBB()),
                               IR::VoidType::get(ctx->llctx), false, IR::Nature::pure);
        }
      } else {
        ctx->Error("It is compulsory to provide the tagged name of the loop in a "
                   "continue sentence, if there are nested loops",
                   fileRange);
      }
    }
  }
  return nullptr;
}

Json Continue::toJson() const {
  return Json()._("hasTag", tag.has_value())._("tag", tag.has_value() ? tag.value() : JsonValue());
}

} // namespace qat::ast