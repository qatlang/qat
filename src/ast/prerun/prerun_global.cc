#include "./prerun_global.hpp"
#include "../../IR/global_entity.hpp"

namespace qat::ast {

void PrerunGlobal::create_entity(ir::Mod* mod, ir::Ctx* irCtx) {
  mod->entity_name_check(irCtx, name, ir::EntityType::prerunGlobal);
  entityState = mod->add_entity(name, ir::EntityType::prerunGlobal, this, ir::EmitPhase::phase_1);
}

void PrerunGlobal::update_entity_dependencies(ir::Mod* parent, ir::Ctx* irCtx) {
  auto emitCtx = EmitCtx::get(irCtx, parent);
  if (type.has_value()) {
    type.value()->update_dependencies(ir::EmitPhase::phase_1, ir::DependType::complete, entityState, emitCtx);
  }
  value->update_dependencies(ir::EmitPhase::phase_1, ir::DependType::complete, entityState, emitCtx);
}

void PrerunGlobal::do_phase(ir::EmitPhase phase, ir::Mod* parent, ir::Ctx* irCtx) { define(parent, irCtx); }

void PrerunGlobal::define(ir::Mod* mod, ir::Ctx* irCtx) const {
  auto emitCtx = EmitCtx::get(irCtx, mod);
  emitCtx->name_check_in_module(name, "prerun global entity", None);
  ir::Type* valTy = type.has_value() ? type.value()->emit(emitCtx) : nullptr;
  if (type.has_value() && value->has_type_inferrance()) {
    value->as_type_inferrable()->set_inference_type(valTy);
  }
  auto resVal = value->emit(emitCtx);
  if (type.has_value()) {
    if (!valTy->is_same(resVal->get_ir_type())) {
      irCtx->Error("The provided type for the prerun global is " + irCtx->color(valTy->to_string()) +
                       " but the value is of type " + irCtx->color(resVal->get_ir_type()->to_string()),
                   value->fileRange);
    }
  } else {
    valTy = resVal->get_ir_type();
  }
  new ir::PrerunGlobal(mod, name, valTy, resVal->get_llvm_constant(), emitCtx->getVisibInfo(visibSpec), name.range);
}

Json PrerunGlobal::to_json() const {
  return Json()
      ._("nodeType", "prerunGlobal")
      ._("name", name)
      ._("hasType", type.has_value())
      ._("type", type.has_value() ? type.value()->to_json() : JsonValue())
      ._("value", value->to_json())
      ._("fileRange", fileRange);
}

} // namespace qat::ast
