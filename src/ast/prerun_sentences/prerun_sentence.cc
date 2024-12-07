#include "./prerun_sentence.hpp"
#include "../expression.hpp"

namespace qat::ast {

void emit_prerun_sentences(Vec<PrerunSentence*>& sentences, EmitCtx* ctx) noexcept(false) {
	ctx->prerunCallState->increment_emit_nesting();
	FnAtEnd defer([&]() { ctx->prerunCallState->decrement_emit_nesting(); });
	for (auto snt : sentences) {
		snt->emit(ctx);
	}
}

} // namespace qat::ast
