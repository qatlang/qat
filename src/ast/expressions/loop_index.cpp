#include "./loop_index.hpp"

namespace qat::ast {

LoopIndex::LoopIndex(String _indexName, utils::FileRange _fileRange)
    : Expression(std::move(_fileRange)), indexName(std::move(_indexName)) {}

bool LoopIndex::hasName() const { return !indexName.empty(); }

IR::Value *LoopIndex::emit(IR::Context *ctx) {
  if (getExpectedKind() == ExpressionKind::assignable) {
    ctx->Error("loop'index is not assignable", fileRange);
  }
  if (!ctx->loopsInfo.empty()) {
    if (hasName()) {
      for (const auto &info : ctx->loopsInfo) {
        if (info->isTimes() && info->name == indexName) {
          return new IR::Value(
              ctx->builder.CreateLoad(info->index->getType()->getLLVMType(),
                                      info->index->getAlloca()),
              info->index->getType(), false, IR::Nature::temporary);
        }
      }
      ctx->Error("No loop counter found with the specified name: " +
                     ctx->highlightError(indexName),
                 fileRange);
    } else {
      const auto &info = ctx->loopsInfo.back();
      if (info->type == IR::LoopType::times) {
        return new IR::Value(
            ctx->builder.CreateLoad(info->index->getType()->getLLVMType(),
                                    info->index->getAlloca()),
            info->index->getType(), false, IR::Nature::temporary);
      } else {
        // FIXME - Support "loop over"
        ctx->Error("The loop this expression is present in is not of \"loop "
                   "times\" kind",
                   fileRange);
      }
    }
  } else {
    ctx->Error("This expression is not inside any loop and hence cannot use "
               "loop'index syntax",
               fileRange);
  }
}

nuo::Json LoopIndex::toJson() const {
  return nuo::Json()._("nodeType", "loopIndex");
}

} // namespace qat::ast