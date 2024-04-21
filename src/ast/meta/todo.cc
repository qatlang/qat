#include "./todo.hpp"
#include "../../IR/context.hpp"
#include "../../cli/config.hpp"
#include "llvm/IR/Instructions.h"

namespace qat::ast {

void PrerunMetaTodo::emit(EmitCtx* ctx) {
  auto cfg = cli::Config::get();
  if (cfg->is_build_mode_debug()) {
    ctx->irCtx->Warning("TODO" + (message.has_value() ? (": " + message.value()) : ""), fileRange);
  } else {
    ctx->Error("TODO" + (message.has_value() ? (": " + message.value()) : ""), fileRange);
  }
  if (ctx->has_pre_call_state()) {
    // FIXME - Change so that individual blocks can be tagged as having TODOs
    ctx->get_pre_call_state()->set_given_value(nullptr);
  }
}

ir::Value* MetaTodo::emit(EmitCtx* ctx) {
  auto cfg = cli::Config::get();
  if (cfg->is_build_mode_debug()) {
    ctx->irCtx->Warning("TODO" + (message.has_value() ? (": " + message.value()) : ""), fileRange);
  } else {
    ctx->Error("TODO" + (message.has_value() ? (": " + message.value()) : ""), fileRange);
  }
  if (ctx->has_fn()) {
    if (ctx->get_fn()->get_block()->get_bb()->empty() ||
        !llvm::isa<llvm::UnreachableInst>(*ctx->get_fn()->get_block()->get_bb()->end())) {
      ctx->irCtx->builder.CreateUnreachable();
    }
    ctx->get_fn()->get_block()->set_has_todo();
  }
  return nullptr;
}

Json MetaTodo::to_json() const {
  return Json()._("hasMessage", message.has_value())._("message", message.has_value() ? message.value() : JsonValue());
}

} // namespace qat::ast