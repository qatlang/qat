#include "./loop_index.hpp"

namespace qat::ast {

bool TagOfLoop::hasName() const { return !indexName.empty(); }

IR::Value* TagOfLoop::emit(IR::Context* ctx) {
  if (!ctx->loopsInfo.empty()) {
    if (hasName()) {
      for (const auto& info : ctx->loopsInfo) {
        if (info.isTimes() && info.name == indexName) {
          return new IR::Value(ctx->builder.CreateLoad(info.index->getType()->getLLVMType(), info.index->getAlloca()),
                               info.index->getType(), false, IR::Nature::temporary);
        }
      }
      ctx->Error("No loop counter found with the specified name: " + ctx->highlightError(indexName), fileRange);
    } else {
      const auto& info = ctx->loopsInfo.back();
      if (info.type == IR::LoopType::nTimes) {
        return new IR::Value(ctx->builder.CreateLoad(info.index->getType()->getLLVMType(), info.index->getAlloca()),
                             info.index->getType(), false, IR::Nature::temporary);
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

Json TagOfLoop::toJson() const { return Json()._("nodeType", "tagOfLoop"); }

} // namespace qat::ast