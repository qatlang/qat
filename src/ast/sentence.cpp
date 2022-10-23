#include "./sentence.hpp"
#include "../IR/control_flow.hpp"

namespace qat::ast {

void emitSentences(const Vec<Sentence*>& sentences, IR::Context* ctx) {
  for (auto* snt : sentences) {
    SHOW("Sentence nodeType is: " << (int)snt->nodeType())
    auto* irVal = snt->emit(ctx);
    if (irVal && irVal->getLLVM() && IR::isTerminatorInstruction(irVal->getLLVM())) {
      break;
    }
  }
}

} // namespace qat::ast