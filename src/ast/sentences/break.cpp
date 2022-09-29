#include "./break.hpp"
#include "../../IR/control_flow.hpp"

namespace qat::ast {

Break::Break(Maybe<String> _tag, utils::FileRange _fileRange)
    : Sentence(std::move(_fileRange)), tag(std::move(_tag)) {}

IR::Value *Break::emit(IR::Context *ctx) {
  if (ctx->breakables.empty()) {
    ctx->Error("Break sentence is not present inside any loop or switch blocks",
               fileRange);
  } else {
    if (tag.has_value()) {
      for (auto *brk : ctx->breakables) {
        if (brk->tag.has_value()) {
          if (tag.value() == brk->tag.value()) {
            return new IR::Value(
                IR::addBranch(ctx->builder, brk->restBlock->getBB()),
                IR::VoidType::get(ctx->llctx), false, IR::Nature::pure);
          }
        }
      }
      ctx->Error("The provided tag " + ctx->highlightError(tag.value()) +
                     " does not match the tag of any parent loops or switches",
                 fileRange);
    } else {
      if (ctx->breakables.size() == 1) {
        return new IR::Value(
            IR::addBranch(ctx->builder,
                          ctx->breakables.front()->restBlock->getBB()),
            IR::VoidType::get(ctx->llctx), false, IR::Nature::pure);
      } else {
        ctx->Error(
            "It is compulsory to provide the tagged name of the loop or switch "
            "in a break sentence, if there is a nesting of loops or switches",
            fileRange);
      }
    }
  }
  return nullptr;
}

Json Break::toJson() const {
  return Json()._("hasTag", tag.has_value())._("tag", tag.value_or(""));
}

} // namespace qat::ast