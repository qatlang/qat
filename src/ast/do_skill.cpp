#include "./do_skill.hpp"
#include "../IR/skill.hpp"
#include "constructor.hpp"
#include "convertor.hpp"

namespace qat::ast {

void DoSkill::create_entity(IR::QatModule* parent, IR::Context* ctx) {
  if (isDefaultSkill) {
    entityState = parent->add_entity(None, IR::EntityType::defaultDoneSkill, this, IR::EmitPhase::phase_3);
    entityState->phaseToPartial = IR::EmitPhase::phase_2;
    for (auto memFn : memberDefinitions) {
      memFn->prototype->add_to_parent(entityState, ctx);
    }
  }
}
void DoSkill::update_entity_dependencies(IR::QatModule* parent, IR::Context* ctx) {
  if (isDefaultSkill) {
    targetType->update_dependencies(IR::EmitPhase::phase_1, IR::DependType::complete, entityState, ctx);
    // targetType->update_dependencies(IR::EmitPhase::phase_2, IR::DependType::childrenPartial, entityState, ctx);
    if (defaultConstructor) {
      defaultConstructor->prototype->update_dependencies(IR::EmitPhase::phase_2, IR::DependType::complete, entityState,
                                                         ctx);
      defaultConstructor->update_dependencies(IR::EmitPhase::phase_3, IR::DependType::complete, entityState, ctx);
    }
    if (copyConstructor) {
      copyConstructor->prototype->update_dependencies(IR::EmitPhase::phase_2, IR::DependType::complete, entityState,
                                                      ctx);
      copyConstructor->update_dependencies(IR::EmitPhase::phase_3, IR::DependType::complete, entityState, ctx);
    }
    if (moveConstructor) {
      moveConstructor->prototype->update_dependencies(IR::EmitPhase::phase_2, IR::DependType::complete, entityState,
                                                      ctx);
      moveConstructor->update_dependencies(IR::EmitPhase::phase_3, IR::DependType::complete, entityState, ctx);
    }
    if (copyAssignment) {
      copyAssignment->prototype->update_dependencies(IR::EmitPhase::phase_2, IR::DependType::complete, entityState,
                                                     ctx);
      copyAssignment->update_dependencies(IR::EmitPhase::phase_3, IR::DependType::complete, entityState, ctx);
    }
    if (moveAssignment) {
      moveAssignment->prototype->update_dependencies(IR::EmitPhase::phase_2, IR::DependType::complete, entityState,
                                                     ctx);
      moveAssignment->update_dependencies(IR::EmitPhase::phase_3, IR::DependType::complete, entityState, ctx);
    }
    if (destructorDefinition) {
      destructorDefinition->update_dependencies(IR::EmitPhase::phase_3, IR::DependType::complete, entityState, ctx);
    }
    for (auto cons : constructorDefinitions) {
      cons->prototype->update_dependencies(IR::EmitPhase::phase_2, IR::DependType::complete, entityState, ctx);
      cons->update_dependencies(IR::EmitPhase::phase_3, IR::DependType::complete, entityState, ctx);
    }
    for (auto conv : convertorDefinitions) {
      conv->prototype->update_dependencies(IR::EmitPhase::phase_2, IR::DependType::complete, entityState, ctx);
      conv->update_dependencies(IR::EmitPhase::phase_3, IR::DependType::complete, entityState, ctx);
    }
    for (auto memFn : memberDefinitions) {
      memFn->prototype->update_dependencies(IR::EmitPhase::phase_2, IR::DependType::complete, entityState, ctx);
      memFn->update_dependencies(IR::EmitPhase::phase_3, IR::DependType::complete, entityState, ctx);
    }
    for (auto opr : operatorDefinitions) {
      opr->prototype->update_dependencies(IR::EmitPhase::phase_2, IR::DependType::complete, entityState, ctx);
      opr->update_dependencies(IR::EmitPhase::phase_3, IR::DependType::complete, entityState, ctx);
    }
  }
}
void DoSkill::do_phase(IR::EmitPhase phase, IR::QatModule* parent, IR::Context* ctx) {
  if (phase == IR::EmitPhase::phase_1) {
    define_done_skill(parent, ctx);
  } else if (phase == IR::EmitPhase::phase_2) {
    define_members(ctx);
  } else if (phase == IR::EmitPhase::phase_3) {
    emit_members(ctx);
  }
}

void DoSkill::define_done_skill(IR::QatModule* mod, IR::Context* ctx) {
  auto* target = targetType->emit(ctx);
  if (target->isRegion() || target->isReference() || target->isTyped() || target->isFunction() || target->isVoid()) {
    ctx->Error("Creating a default implementation for " + ctx->highlightError(target->toString()) + " is not allowed",
               fileRange);
  }
  if (isDefaultSkill) {
    doneSkill = IR::DoneSkill::create_default(mod, fileRange, target, targetType->fileRange);
    if (has_copy_constructor()) {
      ctx->Error(
          "Copy constructor is not allowed in default implementations, but only in the original type, if the type is a struct type",
          copyConstructor->fileRange);
    }
    if (has_copy_assignment()) {
      ctx->Error(
          "Copy assignment operator is not allowed in default implementations, but only in the original type, if the type is a struct type",
          copyAssignment->fileRange);
    }
    if (has_move_constructor()) {
      ctx->Error(
          "Move constructor is not allowed in default implementations, but only in the original type, if the type is a struct type",
          moveConstructor->fileRange);
    }
    if (has_move_assignment()) {
      ctx->Error(
          "Move assignment operator is not allowed in default implementations, but only in the original type, if the type is a struct type",
          moveAssignment->fileRange);
    }
    if (has_destructor()) {
      ctx->Error(
          "Destructor is not allowed in default implementations, but only in the original type, if the type is a struct type",
          destructorDefinition->fileRange);
    }
  } else {
    ctx->Error("Implementing skills is not supported for now", fileRange);
  }
}

void DoSkill::define_members(IR::Context* ctx) {
  parent           = IR::MemberParent::create_do_skill(doneSkill);
  auto parentState = get_state_for(parent);
  if (has_default_constructor()) {
    ctx->set_active_done_skill(doneSkill);
    MethodState state(parent);
    defaultConstructor->define(state, ctx);
    parentState->defaultConstructor = state.result;
    ctx->unset_active_done_skill();
  }
  if (has_copy_constructor()) {
    ctx->set_active_done_skill(doneSkill);
    MethodState state(parent);
    copyConstructor->define(state, ctx);
    parentState->copyConstructor = state.result;
    ctx->unset_active_done_skill();
  }
  if (has_copy_assignment()) {
    ctx->set_active_done_skill(doneSkill);
    MethodState state(parent);
    copyAssignment->define(state, ctx);
    parentState->copyAssignment = state.result;
    ctx->unset_active_done_skill();
  }
  if (has_move_constructor()) {
    ctx->set_active_done_skill(doneSkill);
    MethodState state(parent);
    moveConstructor->define(state, ctx);
    parentState->moveConstructor = state.result;
    ctx->unset_active_done_skill();
  }
  if (has_move_assignment()) {
    ctx->set_active_done_skill(doneSkill);
    MethodState state(parent);
    moveAssignment->define(state, ctx);
    parentState->moveAssignment = state.result;
    ctx->unset_active_done_skill();
  }
  if (has_destructor()) {
    ctx->set_active_done_skill(doneSkill);
    MethodState state(parent);
    destructorDefinition->define(state, ctx);
    parentState->destructor = state.result;
    ctx->unset_active_done_skill();
  }
  for (auto* conv : convertorDefinitions) {
    ctx->set_active_done_skill(doneSkill);
    MethodState state(parent);
    conv->define(state, ctx);
    parentState->convertors.push_back(state.result);
    ctx->unset_active_done_skill();
  }
  for (auto* cons : constructorDefinitions) {
    ctx->set_active_done_skill(doneSkill);
    MethodState state(parent);
    cons->define(state, ctx);
    parentState->constructors.push_back(state.result);
    ctx->unset_active_done_skill();
  }
  for (auto* memDef : memberDefinitions) {
    ctx->set_active_done_skill(doneSkill);
    MethodState state(parent);
    memDef->define(state, ctx);
    parentState->all_methods.push_back(MethodResult(state.result, state.defineCondition));
    ctx->unset_active_done_skill();
  }
  for (auto* oprDef : operatorDefinitions) {
    ctx->set_active_done_skill(doneSkill);
    MethodState state(parent);
    oprDef->define(state, ctx);
    parentState->operators.push_back(state.result);
    ctx->unset_active_done_skill();
  }
}

void DoSkill::emit_members(IR::Context* ctx) {
  parent           = IR::MemberParent::create_do_skill(doneSkill);
  auto parentState = get_state_for(parent);
  if (defaultConstructor) {
    ctx->set_active_done_skill(doneSkill);
    MethodState state(parent, parentState->defaultConstructor);
    (void)defaultConstructor->emit(state, ctx);
    parentState->defaultConstructor = state.result;
    ctx->unset_active_done_skill();
  }
  if (copyConstructor) {
    ctx->set_active_done_skill(doneSkill);
    MethodState state(parent, parentState->copyConstructor);
    (void)copyConstructor->emit(state, ctx);
    parentState->copyConstructor = state.result;
    ctx->unset_active_done_skill();
  }
  if (moveConstructor) {
    ctx->set_active_done_skill(doneSkill);
    MethodState state(parent, parentState->moveConstructor);
    (void)moveConstructor->emit(state, ctx);
    parentState->moveConstructor = state.result;
    ctx->unset_active_done_skill();
  }
  if (copyAssignment) {
    ctx->set_active_done_skill(doneSkill);
    MethodState state(parent, parentState->copyAssignment);
    (void)copyAssignment->emit(state, ctx);
    parentState->copyAssignment = state.result;
    ctx->unset_active_done_skill();
  }
  if (moveAssignment) {
    ctx->set_active_done_skill(doneSkill);
    MethodState state(parent, parentState->moveAssignment);
    (void)moveAssignment->emit(state, ctx);
    parentState->moveAssignment = state.result;
    ctx->unset_active_done_skill();
  }
  for (usize i = 0; i < constructorDefinitions.size(); i++) {
    ctx->set_active_done_skill(doneSkill);
    MethodState state(parent, parentState->constructors[i]);
    (void)constructorDefinitions[i]->emit(state, ctx);
    ctx->unset_active_done_skill();
  }
  for (usize i = 0; i < convertorDefinitions.size(); i++) {
    ctx->set_active_done_skill(doneSkill);
    MethodState state(parent, parentState->convertors[i]);
    (void)convertorDefinitions[i]->emit(state, ctx);
    ctx->unset_active_done_skill();
  }
  for (usize i = 0; i < memberDefinitions.size(); i++) {
    ctx->set_active_done_skill(doneSkill);
    MethodState state(parent, parentState->all_methods[i].fn, parentState->all_methods[i].condition);
    (void)memberDefinitions[i]->emit(state, ctx);
    ctx->unset_active_done_skill();
  }
  for (usize i = 0; i < operatorDefinitions.size(); i++) {
    ctx->set_active_done_skill(doneSkill);
    MethodState state(parent, parentState->operators[i]);
    (void)operatorDefinitions[i]->emit(state, ctx);
    ctx->unset_active_done_skill();
  }
  if (destructorDefinition) {
    ctx->set_active_done_skill(doneSkill);
    MethodState state(parent, parentState->destructor);
    (void)destructorDefinition->emit(state, ctx);
    ctx->unset_active_done_skill();
  }
  // TODO - Member function call tree analysis
}

Json DoSkill::toJson() const {
  return Json()._("isDefault", isDefaultSkill)._("targetType", targetType->toJson())._("fileRange", fileRange);
}

} // namespace qat::ast