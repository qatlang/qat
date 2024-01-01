#include "./sentence.hpp"
#include "../IR/control_flow.hpp"

namespace qat::ast {

void emitSentences(const Vec<Sentence*>& sentences, IR::Context* ctx) {
  for (auto* snt : sentences) {
    if (ctx->getActiveFunction()->getBlock()->fileRange) {
      ctx->getActiveFunction()->getBlock()->fileRange =
          ctx->getActiveFunction()->getBlock()->fileRange.value().trimTo(snt->fileRange.start);
    }
    auto* irVal = snt->emit(ctx);
    if (irVal && irVal->getLLVM() && IR::isTerminatorInstruction(irVal->getLLVM())) {
      break;
    }
  }
}

} // namespace qat::ast