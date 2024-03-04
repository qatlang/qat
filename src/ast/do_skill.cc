#include "./do_skill.hpp"
#include "../IR/skill.hpp"
#include "constructor.hpp"
#include "convertor.hpp"

namespace qat::ast {

void DoSkill::create_entity(ir::Mod* parent, ir::Ctx* irCtx) {
  if (isDefaultSkill) {
    entityState = parent->add_entity(None, ir::EntityType::defaultDoneSkill, this, ir::EmitPhase::phase_3);
    entityState->phaseToPartial = ir::EmitPhase::phase_2;
    for (auto memFn : memberDefinitions) {
      memFn->prototype->add_to_parent(entityState, irCtx);
    }
  }
}
void DoSkill::update_entity_dependencies(ir::Mod* parent, ir::Ctx* irCtx) {
  if (isDefaultSkill) {
    auto ctx = EmitCtx::get(irCtx, parent);
    targetType->update_dependencies(ir::EmitPhase::phase_1, ir::DependType::complete, entityState, ctx);
    if (defaultConstructor) {
      defaultConstructor->prototype->update_dependencies(ir::EmitPhase::phase_2, ir::DependType::complete, entityState,
                                                         ctx);
      defaultConstructor->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState, ctx);
    }
    if (copyConstructor) {
      copyConstructor->prototype->update_dependencies(ir::EmitPhase::phase_2, ir::DependType::complete, entityState,
                                                      ctx);
      copyConstructor->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState, ctx);
    }
    if (moveConstructor) {
      moveConstructor->prototype->update_dependencies(ir::EmitPhase::phase_2, ir::DependType::complete, entityState,
                                                      ctx);
      moveConstructor->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState, ctx);
    }
    if (copyAssignment) {
      copyAssignment->prototype->update_dependencies(ir::EmitPhase::phase_2, ir::DependType::complete, entityState,
                                                     ctx);
      copyAssignment->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState, ctx);
    }
    if (moveAssignment) {
      moveAssignment->prototype->update_dependencies(ir::EmitPhase::phase_2, ir::DependType::complete, entityState,
                                                     ctx);
      moveAssignment->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState, ctx);
    }
    if (destructorDefinition) {
      destructorDefinition->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState, ctx);
    }
    for (auto cons : constructorDefinitions) {
      cons->prototype->update_dependencies(ir::EmitPhase::phase_2, ir::DependType::complete, entityState, ctx);
      cons->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState, ctx);
    }
    for (auto conv : convertorDefinitions) {
      conv->prototype->update_dependencies(ir::EmitPhase::phase_2, ir::DependType::complete, entityState, ctx);
      conv->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState, ctx);
    }
    for (auto memFn : memberDefinitions) {
      memFn->prototype->update_dependencies(ir::EmitPhase::phase_2, ir::DependType::complete, entityState, ctx);
      memFn->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState, ctx);
    }
    for (auto opr : operatorDefinitions) {
      opr->prototype->update_dependencies(ir::EmitPhase::phase_2, ir::DependType::complete, entityState, ctx);
      opr->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState, ctx);
    }
  }
}
void DoSkill::do_phase(ir::EmitPhase phase, ir::Mod* parent, ir::Ctx* irCtx) {
  if (phase == ir::EmitPhase::phase_1) {
    define_done_skill(parent, irCtx);
  } else if (phase == ir::EmitPhase::phase_2) {
    define_members(irCtx);
  } else if (phase == ir::EmitPhase::phase_3) {
    emit_members(irCtx);
  }
}

void DoSkill::define_done_skill(ir::Mod* mod, ir::Ctx* irCtx) {
  auto* target = targetType->emit(EmitCtx::get(irCtx, mod));
  if (target->is_region() || target->is_reference() || target->is_typed() || target->is_function() ||
      target->is_void()) {
    irCtx->Error("Creating a default implementation for " + irCtx->color(target->to_string()) + " is not allowed",
                 fileRange);
  }
  if (isDefaultSkill) {
    doneSkill = ir::DoneSkill::create_extension(mod, fileRange, target, targetType->fileRange);
    if (has_copy_constructor()) {
      irCtx->Error(
          "Copy constructor is not allowed in default implementations, but only in the original type, if the type is a struct type",
          copyConstructor->fileRange);
    }
    if (has_copy_assignment()) {
      irCtx->Error(
          "Copy assignment operator is not allowed in default implementations, but only in the original type, if the type is a struct type",
          copyAssignment->fileRange);
    }
    if (has_move_constructor()) {
      irCtx->Error(
          "Move constructor is not allowed in default implementations, but only in the original type, if the type is a struct type",
          moveConstructor->fileRange);
    }
    if (has_move_assignment()) {
      irCtx->Error(
          "Move assignment operator is not allowed in default implementations, but only in the original type, if the type is a struct type",
          moveAssignment->fileRange);
    }
    if (has_destructor()) {
      irCtx->Error(
          "Destructor is not allowed in default implementations, but only in the original type, if the type is a struct type",
          destructorDefinition->fileRange);
    }
  } else {
    irCtx->Error("Implementing skills is not supported for now", fileRange);
  }
}

void DoSkill::define_members(ir::Ctx* irCtx) {
  parent           = ir::MemberParent::create_do_skill(doneSkill);
  auto parentState = get_state_for(parent);
  if (has_default_constructor()) {
    MethodState state(parent);
    defaultConstructor->define(state, irCtx);
    parentState->defaultConstructor = state.result;
  }
  if (has_copy_constructor()) {
    MethodState state(parent);
    copyConstructor->define(state, irCtx);
    parentState->copyConstructor = state.result;
  }
  if (has_copy_assignment()) {
    MethodState state(parent);
    copyAssignment->define(state, irCtx);
    parentState->copyAssignment = state.result;
  }
  if (has_move_constructor()) {
    MethodState state(parent);
    moveConstructor->define(state, irCtx);
    parentState->moveConstructor = state.result;
  }
  if (has_move_assignment()) {
    MethodState state(parent);
    moveAssignment->define(state, irCtx);
    parentState->moveAssignment = state.result;
  }
  if (has_destructor()) {
    MethodState state(parent);
    destructorDefinition->define(state, irCtx);
    parentState->destructor = state.result;
  }
  for (auto* conv : convertorDefinitions) {
    MethodState state(parent);
    conv->define(state, irCtx);
    parentState->convertors.push_back(state.result);
  }
  for (auto* cons : constructorDefinitions) {
    MethodState state(parent);
    cons->define(state, irCtx);
    parentState->constructors.push_back(state.result);
  }
  for (auto* memDef : memberDefinitions) {
    MethodState state(parent);
    memDef->define(state, irCtx);
    parentState->all_methods.push_back(MethodResult(state.result, state.defineCondition));
  }
  for (auto* oprDef : operatorDefinitions) {
    MethodState state(parent);
    oprDef->define(state, irCtx);
    parentState->operators.push_back(state.result);
  }
}

void DoSkill::emit_members(ir::Ctx* irCtx) {
  parent           = ir::MemberParent::create_do_skill(doneSkill);
  auto parentState = get_state_for(parent);
  if (defaultConstructor) {
    MethodState state(parent, parentState->defaultConstructor);
    (void)defaultConstructor->emit(state, irCtx);
  }
  if (copyConstructor) {
    MethodState state(parent, parentState->copyConstructor);
    (void)copyConstructor->emit(state, irCtx);
  }
  if (moveConstructor) {
    MethodState state(parent, parentState->moveConstructor);
    (void)moveConstructor->emit(state, irCtx);
  }
  if (copyAssignment) {
    MethodState state(parent, parentState->copyAssignment);
    (void)copyAssignment->emit(state, irCtx);
  }
  if (moveAssignment) {
    MethodState state(parent, parentState->moveAssignment);
    (void)moveAssignment->emit(state, irCtx);
  }
  for (usize i = 0; i < constructorDefinitions.size(); i++) {
    MethodState state(parent, parentState->constructors[i]);
    (void)constructorDefinitions[i]->emit(state, irCtx);
  }
  for (usize i = 0; i < convertorDefinitions.size(); i++) {
    MethodState state(parent, parentState->convertors[i]);
    (void)convertorDefinitions[i]->emit(state, irCtx);
  }
  for (usize i = 0; i < memberDefinitions.size(); i++) {
    MethodState state(parent, parentState->all_methods[i].fn, parentState->all_methods[i].condition);
    (void)memberDefinitions[i]->emit(state, irCtx);
  }
  for (usize i = 0; i < operatorDefinitions.size(); i++) {
    MethodState state(parent, parentState->operators[i]);
    (void)operatorDefinitions[i]->emit(state, irCtx);
  }
  if (destructorDefinition) {
    MethodState state(parent, parentState->destructor);
    (void)destructorDefinition->emit(state, irCtx);
  }
  // TODO - Member function call tree analysis
}

Json DoSkill::to_json() const {
  return Json()._("isDefault", isDefaultSkill)._("targetType", targetType->to_json())._("fileRange", fileRange);
}

} // namespace qat::ast