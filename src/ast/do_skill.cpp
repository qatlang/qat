#include "./do_skill.hpp"
#include "../IR/skill.hpp"
#include "constructor.hpp"
#include "convertor.hpp"

namespace qat::ast {

void DoSkill::defineType(IR::Context* ctx) {
  auto* mod    = ctx->getMod();
  auto* target = targetType->emit(ctx);
  if (target->isRegion() || target->isReference() || target->isTyped() || target->isFunction() || target->isVoid()) {
    ctx->Error("Creating a default implementation for " + ctx->highlightError(target->toString()) + " is not allowed",
               fileRange);
  }
  if (isDefaultSkill) {
    doneSkill   = IR::DoneSkill::create_default(mod, fileRange, target, targetType->fileRange);
    auto parent = IR::MemberParent::create_do_skill(doneSkill);
    if (has_default_constructor()) {
      defaultConstructor->setMemberParent(parent);
    }
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
    for (auto* conv : convertorDefinitions) {
      conv->setMemberParent(parent);
    }
    for (auto* cons : constructorDefinitions) {
      cons->setMemberParent(parent);
    }
    for (auto* memDef : memberDefinitions) {
      memDef->setMemberParent(parent);
    }
    for (auto* oprDef : operatorDefinitions) {
      oprDef->setMemberParent(parent);
    }
  } else {
    ctx->Error("Implementing skills is not supported for now", fileRange);
  }
}

void DoSkill::define(IR::Context* ctx) {
  if (has_default_constructor()) {
    defaultConstructor->define(ctx);
  }
  if (has_copy_constructor()) {
    copyConstructor->define(ctx);
  }
  if (has_copy_assignment()) {
    copyAssignment->define(ctx);
  }
  if (has_move_constructor()) {
    moveConstructor->define(ctx);
  }
  if (has_move_assignment()) {
    moveAssignment->define(ctx);
  }
  if (has_destructor()) {
    destructorDefinition->define(ctx);
  }
  for (auto* conv : convertorDefinitions) {
    conv->define(ctx);
  }
  for (auto* cons : constructorDefinitions) {
    cons->define(ctx);
  }
  for (auto* memDef : memberDefinitions) {
    memDef->define(ctx);
  }
  for (auto* oprDef : operatorDefinitions) {
    oprDef->define(ctx);
  }
}

IR::Value* DoSkill::emit(IR::Context* ctx) {
  if (defaultConstructor) {
    (void)defaultConstructor->emit(ctx);
  }
  if (copyConstructor) {
    (void)copyConstructor->emit(ctx);
  }
  if (moveConstructor) {
    (void)moveConstructor->emit(ctx);
  }
  for (auto* cons : constructorDefinitions) {
    (void)cons->emit(ctx);
  }
  for (auto* conv : convertorDefinitions) {
    (void)conv->emit(ctx);
  }
  for (auto* mFn : memberDefinitions) {
    (void)mFn->emit(ctx);
  }
  for (auto* oFn : operatorDefinitions) {
    (void)oFn->emit(ctx);
  }
  if (copyAssignment) {
    (void)copyAssignment->emit(ctx);
  }
  if (moveAssignment) {
    (void)moveAssignment->emit(ctx);
  }
  if (destructorDefinition) {
    (void)destructorDefinition->emit(ctx);
  }
  // TODO - Member function call tree analysis
  return nullptr;
}

Json DoSkill::toJson() const {
  return Json()._("isDefault", isDefaultSkill)._("targetType", targetType->toJson())._("fileRange", fileRange);
}

} // namespace qat::ast