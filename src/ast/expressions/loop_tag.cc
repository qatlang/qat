#include "./loop_tag.hpp"

namespace qat::ast {

bool TagOfLoop::hasName() const { return !indexName.empty(); }

ir::Value* TagOfLoop::emit(EmitCtx* ctx) {
  if (!ctx->loopsInfo.empty()) {
    if (hasName()) {
      for (const auto& info : ctx->loopsInfo) {
        if (info.isTimes() && info.name == indexName) {
          return ir::Value::get(
              ctx->irCtx->builder.CreateLoad(info.index->get_ir_type()->get_llvm_type(), info.index->getAlloca()),
              info.index->get_ir_type(), false);
        }
      }
      ctx->Error("No loop counter found with the specified name: " + ctx->color(indexName), fileRange);
    } else {
      const auto& info = ctx->loopsInfo.back();
      if (info.type == LoopType::nTimes) {
        return ir::Value::get(
            ctx->irCtx->builder.CreateLoad(info.index->get_ir_type()->get_llvm_type(), info.index->getAlloca()),
            info.index->get_ir_type(), false);
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

Json TagOfLoop::to_json() const { return Json()._("nodeType", "tagOfLoop"); }

} // namespace qat::ast