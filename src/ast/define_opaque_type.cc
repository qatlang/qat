#include "./define_opaque_type.hpp"
#include "../IR/types/opaque.hpp"
#include "node.hpp"

namespace qat::ast {

void DefineOpaqueType::create_entity(ir::Mod* parent, ir::Ctx* irCtx) {
  SHOW("CreateEntity: " << name.value)
  parent->entity_name_check(irCtx, name, ir::EntityType::opaque);
  entityState = parent->add_entity(name, ir::EntityType::opaque, this, ir::EmitPhase::phase_1);
}

void DefineOpaqueType::update_entity_dependencies(ir::Mod* mod, ir::Ctx* irCtx) {
  if (condition.has_value()) {
    condition.value()->update_dependencies(ir::EmitPhase::phase_1, ir::DependType::complete, entityState,
                                           EmitCtx::get(irCtx, mod));
  }
}

void DefineOpaqueType::do_phase(ir::EmitPhase phase, ir::Mod* parent, ir::Ctx* irCtx) {
  SHOW("Defining opaque type " << name.value << " with visibility "
                               << (visibSpec.has_value() ? visibSpec.value().to_string() : ""))
  auto emitCtx = EmitCtx::get(irCtx, parent);
  if (condition.has_value()) {
    auto cond = condition.value()->emit(emitCtx);
    if (cond->get_ir_type()->is_bool()) {
      if (!llvm::cast<llvm::ConstantInt>(cond->get_llvm_constant())->getValue().getBoolValue()) {
        return;
      }
    } else {
      irCtx->Error("The condition for defining this opaque type should be of " + irCtx->color("bool") +
                       " type. Got an expression with type " + irCtx->color(cond->get_ir_type()->to_string()),
                   condition.value()->fileRange);
    }
  }
  emitCtx->name_check_in_module(name, "opaque type", None);
  Maybe<ir::MetaInfo> irMeta;
  if (metaInfo.has_value()) {
    irMeta = metaInfo.value().toIR(emitCtx);
  }
  (void)ir::OpaqueType::get(name, {}, None, ir::OpaqueSubtypeKind::core, parent, None, emitCtx->getVisibInfo(visibSpec),
                            irCtx->llctx, irMeta);
}

Json DefineOpaqueType::to_json() const {
  return Json()
      ._("nodeType", "defineOpaqueType")
      ._("name", name)
      ._("visibility", visibSpec.has_value() ? visibSpec.value().to_json() : JsonValue())
      ._("fileRange", fileRange);
}

} // namespace qat::ast