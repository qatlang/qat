#include "./sentence.hpp"
#include "../IR/control_flow.hpp"

namespace qat::ast {

void emit_sentences(const Vec<Sentence*>& sentences, EmitCtx* ctx) {
  for (auto* snt : sentences) {
    SHOW("Updating block range")
    if (ctx->has_fn()) {
      if (ctx->get_fn()->get_block()->fileRange) {
        ctx->get_fn()->get_block()->fileRange =
            ctx->get_fn()->get_block()->fileRange.value().trimTo(snt->fileRange.start);
      }
    }
    SHOW("Emitting sentence")
    SHOW("Sentence type is " << (int)snt->nodeType())
    auto* irVal = snt->emit(ctx);
    SHOW("Emitted sentence")
    if (irVal && irVal->get_llvm() && ir::is_terminator_instruction(irVal->get_llvm())) {
      break;
    }
  }
}

} // namespace qat::ast