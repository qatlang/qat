#include "./prerun_function.hpp"
#include "../ast/emit_ctx.hpp"
#include "../ast/prerun_sentences/internal_exceptions.hpp"
#include "../ast/prerun_sentences/prerun_sentence.hpp"
#include "context.hpp"

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
  try {
    ast::emit_prerun_sentences(sentences.first, emitCtx);
  } catch (InternalPrerunGive& given) {
    delete callState; // FIXME - Remove when IR part of the codebase uses region allocator
    return given.value;
  } catch (...) {
    irCtx->Error("Failed to call the prerun function with an exception", fileRange);
  }
  if (returnType->is_void()) {
    return nullptr;
  } else {
    irCtx->Error("This prerun function did not give any value", fileRange);
  }
}
}

} // namespace qat::ir
