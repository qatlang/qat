#include "./operator_function.hpp"
#include "../show.hpp"
#include "expressions/operator.hpp"
#include "types/self_type.hpp"
#include "llvm/IR/GlobalValue.h"
#include <vector>

namespace qat::ast {

OperatorPrototype::~OperatorPrototype() {
  for (auto* arg : arguments) {
    std::destroy_at(arg);
  }
}

void OperatorPrototype::define(IR::Context* ctx) {
  if (!memberParent) {
    ctx->Error("No parent type found for this member function", fileRange);
  }
  if (opr == Op::copyAssignment) {
    if (memberParent->isDoneSkill() && memberParent->asDoneSkill()->hasCopyAssignment()) {
      ctx->Error("Copy assignment operator already exists in this implementation " +
                     ctx->highlightError(memberParent->asDoneSkill()->toString()),
                 fileRange);
    } else if (memberParent->isExpanded() && memberParent->asExpanded()->hasCopyAssignment()) {
      ctx->Error("Copy assignment operator already exists for the parent type " +
                     ctx->highlightError(memberParent->asExpanded()->toString()),
                 fileRange);
    }
    memberFn = IR::MemberFunction::CopyAssignment(memberParent, nameRange, argName.value(), fileRange, ctx);
    return;
  } else if (opr == Op::moveAssignment) {
    if (memberParent->isExpanded() && memberParent->asExpanded()->hasMoveAssignment()) {
      ctx->Error("Move assignment operator already exists for the parent type " +
                     ctx->highlightError(memberParent->asExpanded()->getFullName()),
                 fileRange);
    } else if (memberParent->isDoneSkill() && memberParent->asDoneSkill()->hasMoveAssignment()) {
      ctx->Error("Move assignment operator already exists in this implementation " +
                     ctx->highlightError(memberParent->asDoneSkill()->toString()),
                 fileRange);
    }
    memberFn = IR::MemberFunction::MoveAssignment(memberParent, nameRange, argName.value(), fileRange, ctx);
    return;
  }
  if (opr == Op::subtract) {
    if (arguments.empty()) {
      opr = Op::minus;
    }
  }
  if (is_unary_operator(opr)) {
    if (!arguments.empty()) {
      ctx->Error("Unary operators should have no arguments. Invalid definition "
                 "of unary operator " +
                     ctx->highlightError(operator_to_string(opr)),
                 fileRange);
    }
  } else {
    if (arguments.size() != 1) {
      ctx->Error("Invalid number of arguments for Binary operator " + ctx->highlightError(operator_to_string(opr)) +
                     ". Binary operators should have only 1 argument",
                 fileRange);
    }
  }
  Vec<IR::QatType*> generatedTypes;
  SHOW("Generating types")
  for (auto* arg : arguments) {
    if (arg->isTypeMember()) {
      ctx->Error("Member arguments cannot be used in operators", arg->getName().range);
    } else {
      generatedTypes.push_back(arg->getType()->emit(ctx));
    }
  }
  if (is_unary_operator(opr)) {
    if (memberParent->isExpanded() && memberParent->asExpanded()->hasUnaryOperator(operator_to_string(opr))) {
      ctx->Error("Unary operator " + ctx->highlightError(operator_to_string(opr)) +
                     " already exists for the parent type " +
                     ctx->highlightError(memberParent->asExpanded()->getFullName()),
                 fileRange);
    } else if (memberParent->isDoneSkill() && memberParent->asExpanded()->hasUnaryOperator(operator_to_string(opr))) {
      ctx->Error("Unary operator " + ctx->highlightError(operator_to_string(opr)) +
                     " already exists in the implementation " +
                     ctx->highlightError(memberParent->asDoneSkill()->toString()),
                 fileRange);
    }
  } else {
    if (memberParent->isExpanded() && (isVariationFn ? memberParent->asExpanded()->hasVariationBinaryOperator(
                                                           operator_to_string(opr), {None, generatedTypes[0]})
                                                     : memberParent->asExpanded()->hasNormalBinaryOperator(
                                                           operator_to_string(opr), {None, generatedTypes[0]}))) {
      ctx->Error(String(isVariationFn ? "Variation b" : "B") + "inary operator " +
                     ctx->highlightError(operator_to_string(opr)) + " already exists for parent type " +
                     ctx->highlightError(memberParent->asExpanded()->getFullName()) +
                     " with right hand side being type " + ctx->highlightError(generatedTypes.front()->toString()),
                 fileRange);
    } else if (memberParent->isDoneSkill() &&
               (isVariationFn ? memberParent->asDoneSkill()->hasVariationBinaryOperator(operator_to_string(opr),
                                                                                        {None, generatedTypes[0]})
                              : memberParent->asDoneSkill()->hasNormalBinaryOperator(operator_to_string(opr),
                                                                                     {None, generatedTypes[0]}))) {
      ctx->Error(String(isVariationFn ? "Variation b" : "B") + "inary operator " +
                     ctx->highlightError(operator_to_string(opr)) + " already exists in the implementation " +
                     ctx->highlightError(memberParent->asDoneSkill()->toString()) +
                     " with right hand side being type " + ctx->highlightError(generatedTypes.front()->toString()),
                 fileRange);
    }
  }
  SHOW("Argument types generated")
  Vec<IR::Argument> args;
  SHOW("Setting variability of arguments")
  for (usize i = 0; i < generatedTypes.size(); i++) {
    if (arguments.at(i)->isTypeMember()) {
      args.push_back(IR::Argument::CreateMember(arguments.at(i)->getName(), generatedTypes.at(i), i));
    } else {
      args.push_back(
          arguments.at(i)->isVariable()
              ? IR::Argument::CreateVariable(arguments.at(i)->getName(), arguments.at(i)->getType()->emit(ctx), i)
              : IR::Argument::Create(arguments.at(i)->getName(), generatedTypes.at(i), i));
    }
  }
  SHOW("Variability setting complete")
  SHOW("About to create operator function")
  bool isSelfReturn = false;
  if (returnType->typeKind() == AstTypeKind::SELF_TYPE) {
    auto* selfRet = ((SelfType*)returnType);
    if (!selfRet->isJustType) {
      selfRet->isVarRef          = isVariationFn;
      selfRet->canBeSelfInstance = true;
      isSelfReturn               = true;
    }
  }
  auto retTy = returnType->emit(ctx);
  SHOW("Operator " + operator_to_string(opr) + " isVar: " << isVariationFn << " return type is " << retTy->toString())
  memberFn = IR::MemberFunction::CreateOperator(memberParent, nameRange, !is_unary_operator(opr), isVariationFn,
                                                operator_to_string(opr), IR::ReturnType::get(retTy, isSelfReturn), args,
                                                fileRange, ctx->getVisibInfo(visibSpec), ctx);
}

IR::Value* OperatorPrototype::emit(IR::Context* ctx) { return memberFn; }

void OperatorPrototype::setMemberParent(IR::MemberParent* _memberParent) const { memberParent = _memberParent; }

Json OperatorPrototype::toJson() const {
  Vec<JsonValue> args;
  for (auto* arg : arguments) {
    auto aJson = Json()
                     ._("name", arg->getName())
                     ._("type", arg->getType() ? arg->getType()->toJson() : Json())
                     ._("isMemberArg", arg->isTypeMember());
    args.push_back(aJson);
  }
  return Json()
      ._("nodeType", "operatorPrototype")
      ._("isVariation", isVariationFn)
      ._("operator", operator_to_string(opr))
      ._("returnType", returnType->toJson())
      ._("arguments", args)
      ._("hasVisibility", visibSpec.has_value())
      ._("visibility", visibSpec.has_value() ? visibSpec->toJson() : JsonValue())
      ._("fileRange", fileRange);
}

void OperatorDefinition::define(IR::Context* ctx) { prototype->define(ctx); }

IR::Value* OperatorDefinition::emit(IR::Context* ctx) {
  auto* fnEmit = (IR::MemberFunction*)prototype->emit(ctx);
  auto* oldFn  = ctx->setActiveFunction(fnEmit);
  SHOW("Set active operator function: " << fnEmit->getFullName())
  auto* block = new IR::Block(fnEmit, nullptr);
  SHOW("Created entry block")
  block->setActive(ctx->builder);
  SHOW("Set new block as the active block")
  SHOW("About to allocate necessary arguments")
  auto  argIRTypes = fnEmit->getType()->asFunction()->getArgumentTypes();
  auto* coreRefTy  = argIRTypes.at(0)->getType()->asReference();
  auto* self =
      block->newValue("''", coreRefTy, prototype->isVariationFn, coreRefTy->getSubType()->asCore()->getName().range);
  ctx->builder.CreateStore(fnEmit->getLLVMFunction()->getArg(0u), self->getLLVM());
  self->loadImplicitPointer(ctx->builder);
  if ((prototype->opr == Op::copyAssignment) || (prototype->opr == Op::moveAssignment)) {
    auto* argVal =
        block->newValue(prototype->argName->value,
                        IR::ReferenceType::get(prototype->opr == Op::moveAssignment, coreRefTy->getSubType(), ctx),
                        false, prototype->argName->range);
    ctx->builder.CreateStore(fnEmit->getLLVMFunction()->getArg(1u), argVal->getLLVM());
  } else {
    for (usize i = 1; i < argIRTypes.size(); i++) {
      SHOW("Argument type is " << argIRTypes.at(i)->getType()->toString())
      auto* argVal = block->newValue(argIRTypes.at(i)->getName(), argIRTypes.at(i)->getType(), true,
                                     prototype->arguments.at(i - 1)->getName().range);
      SHOW("Created local value for the argument")
      ctx->builder.CreateStore(fnEmit->getLLVMFunction()->getArg(i), argVal->getAlloca(), false);
    }
    SHOW("Operator Return type is " << fnEmit->getType()->asFunction()->getReturnType()->toString())
  }
  emitSentences(sentences, ctx);
  IR::functionReturnHandler(ctx, fnEmit, sentences.empty() ? fileRange : sentences.back()->fileRange);
  SHOW("Sentences emitted")
  (void)ctx->setActiveFunction(oldFn);
  return nullptr;
}

void OperatorDefinition::setMemberParent(IR::MemberParent* memberParent) const {
  prototype->setMemberParent(memberParent);
}

Json OperatorDefinition::toJson() const {
  Vec<JsonValue> sntcs;
  for (auto* sentence : sentences) {
    sntcs.push_back(sentence->toJson());
  }
  return Json()
      ._("nodeType", "operatorDefinition")
      ._("prototype", prototype->toJson())
      ._("body", sntcs)
      ._("fileRange", fileRange);
}

} // namespace qat::ast