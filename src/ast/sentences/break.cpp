#include "./break.hpp"

namespace qat::ast {

Break::Break(Maybe<String> _tag, utils::FileRange _fileRange)
    : Sentence(std::move(_fileRange)), tag(std::move(_tag)) {}

IR::Value *Break::emit(IR::Context *ctx) {
  if (ctx->breakables.empty()) {
    ctx->Error("Break sentence is not present inside any loop or switch blocks",
               fileRange);
  } else {
    if (tag.has_value()) {
      for (usize i = (ctx->breakables.size() - 1); i >= 0; i--) {
        if (ctx->breakables.at(i)->tag.has_value()) {
          if (tag.value() == ctx->breakables.at(i)->tag.value()) {
            ctx->builder.CreateBr(ctx->breakables.at(i)->restBlock->getBB());
            return nullptr;
          }
        }
      }
      ctx->Error("The provided tag " + ctx->highlightError(tag.value()) +
                     " does not match the tag of any parent loops or switches",
                 fileRange);
    } else {
      if (ctx->breakables.size() == 1) {
        ctx->builder.CreateBr(ctx->breakables.front()->restBlock->getBB());
        return nullptr;
      } else {
        ctx->Error(
            "It is compulsory to provide the tagged name of the loop or switch "
            "in a break sentence, if there is a nesting of loops or switches",
            fileRange);
      }
    }
  }
}

nuo::Json Break::toJson() const {
  return nuo::Json()._("hasTag", tag.has_value())._("tag", tag.value_or(""));
}

} // namespace qat::ast