#include "./prerun_sentence.hpp"
#include "../expression.hpp"

namespace qat::ast {

bool emit_prerun_sentences(Vec<PrerunSentence*>& sentences, EmitCtx* ctx) {
  ctx->prerunCallState->increment_emit_nesting();
  FnAtEnd defer([&]() { ctx->prerunCallState->decrement_emit_nesting(); });
  for (auto snt : sentences) {
    snt->emit(ctx);
    if (ctx->get_pre_call_state()->has_given_value()) {
      return true;
    }
  }
  return ctx->get_pre_call_state()->has_given_value();
}

} // namespace qat::ast