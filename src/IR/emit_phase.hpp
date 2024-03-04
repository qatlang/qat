#ifndef QAT_EMIT_PHASE_HPP
#define QAT_EMIT_PHASE_HPP

#include "../utils/helpers.hpp"

namespace qat::ir {
enum class EmitPhase : u8 {
  phase_1,
  phase_2,
  phase_3,
  phase_4,
  phase_5,
  phase_6,
  phase_7,
  phase_8,
  phase_last,
};

static Maybe<EmitPhase> get_next_phase(Maybe<EmitPhase> phase) {
  if (phase.has_value()) {
    switch (phase.value()) {
      case EmitPhase::phase_1:
        return EmitPhase::phase_2;
      case EmitPhase::phase_2:
        return EmitPhase::phase_3;
      case EmitPhase::phase_3:
        return EmitPhase::phase_4;
      case EmitPhase::phase_4:
        return EmitPhase::phase_5;
      case EmitPhase::phase_5:
        return EmitPhase::phase_6;
      case EmitPhase::phase_6:
        return EmitPhase::phase_7;
      case EmitPhase::phase_7:
        return EmitPhase::phase_8;
      case EmitPhase::phase_8:
      case EmitPhase::phase_last:
        return EmitPhase::phase_last;
    }
  } else {
    return EmitPhase::phase_1;
  }
}
} // namespace qat::ir

#endif
