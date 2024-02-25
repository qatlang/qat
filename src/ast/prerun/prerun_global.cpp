#include "./prerun_global.hpp"
#include "../../IR/global_entity.hpp"

namespace qat::ast {

void PrerunGlobal::create_entity(IR::QatModule* mod, IR::Context* ctx) {
  mod->entity_name_check(ctx, name, IR::EntityType::prerunGlobal);
  entityState = mod->add_entity(name, IR::EntityType::prerunGlobal, this, IR::EmitPhase::phase_1);
}

void PrerunGlobal::update_entity_dependencies(IR::QatModule* parent, IR::Context* ctx) {
  if (type.has_value()) {
    type.value()->update_dependencies(IR::EmitPhase::phase_1, IR::DependType::complete, entityState, ctx);
  }
  value->update_dependencies(IR::EmitPhase::phase_1, IR::DependType::complete, entityState, ctx);
}

void PrerunGlobal::do_phase(IR::EmitPhase phase, IR::QatModule* parent, IR::Context* ctx) { define(parent, ctx); }

void PrerunGlobal::define(IR::QatModule* mod, IR::Context* ctx) const {
  ctx->nameCheckInModule(name, "prerun global entity", None);
  IR::QatType* valTy = type.has_value() ? type.value()->emit(ctx) : nullptr;
  if (type.has_value() && value->hasTypeInferrance()) {
    value->asTypeInferrable()->setInferenceType(valTy);
  }
  auto resVal = value->emit(ctx);
  if (type.has_value()) {
    if (!valTy->isSame(resVal->getType())) {
      ctx->Error("The provided type for the prerun global is " + ctx->highlightError(valTy->toString()) +
                     " but the value is of type " + ctx->highlightError(resVal->getType()->toString()),
                 value->fileRange);
    }
  } else {
    valTy = resVal->getType();
  }
  new IR::PrerunGlobal(mod, name, valTy, resVal->getLLVMConstant(), ctx->getVisibInfo(visibSpec), name.range);
}

Json PrerunGlobal::toJson() const {
  return Json()
      ._("nodeType", "prerunGlobal")
      ._("name", name)
      ._("hasType", type.has_value())
      ._("type", type.has_value() ? type.value()->toJson() : JsonValue())
      ._("value", value->toJson())
      ._("fileRange", fileRange);
}

} // namespace qat::ast
