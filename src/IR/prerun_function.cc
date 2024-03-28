#include "./prerun_function.hpp"

#include "../ast/emit_ctx.hpp"
#include "../ast/prerun_sentences/prerun_sentence.hpp"
#include "context.hpp"
#include "types/void.hpp"

namespace qat::ir {

bool PrerunCallState::has_arg_with_name(String const& name) {
  for (auto arg : function->argTypes) {
    if (arg->get_name() == name) {
      return true;
    }
  }
  return false;
}

PrerunValue* PrerunCallState::get_arg_value_for(String const& name) {
  for (usize i = 0; i < function->argTypes.size(); i++) {
    if (function->argTypes[i]->get_name() == name) {
      return argumentValues[i];
    }
  }
  return nullptr;
}

PrerunValue* PrerunFunction::call_prerun(Vec<PrerunValue*> argValues, Ctx* irCtx, FileRange fileRange) {
  auto callState = PrerunCallState::get(this, argValues);
  auto emitCtx   = ast::EmitCtx::get(irCtx, parent)->with_prerun_call_state(callState);
  (void)ast::emit_prerun_sentences(sentences.first, emitCtx);
  if (callState->givenValue.has_value()) {
    return callState->givenValue.value();
  } else {
    if (returnType->is_void()) {
      return PrerunValue::get(nullptr, VoidType::get(irCtx->llctx));
    } else {
      irCtx->Error("No value could be obtained from this prerun function call", fileRange);
    }
  }
  delete callState;
  return nullptr;
}

} // namespace qat::ir