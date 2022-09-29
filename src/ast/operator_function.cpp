#include "./operator_function.hpp"
#include "../show.hpp"
#include "expressions/operator.hpp"
#include "llvm/IR/GlobalValue.h"
#include <vector>

namespace qat::ast {

OperatorPrototype::OperatorPrototype(bool _isVariationFn, Op _op,
                                     Vec<Argument *>         _arguments,
                                     QatType                *_returnType,
                                     utils::VisibilityKind   kind,
                                     const utils::FileRange &_fileRange)
    : Node(_fileRange), isVariationFn(_isVariationFn), opr(_op),
      arguments(std::move(_arguments)), returnType(_returnType), kind(kind) {}

void OperatorPrototype::define(IR::Context *ctx) {
  if (!coreType) {
    ctx->Error("No core type found for this member function", fileRange);
  }
  if (opr == Op::subtract) {
    if (arguments.empty()) {
      opr = Op::minus;
    }
  }
  if (isUnaryOp(opr)) {
    if (!arguments.empty()) {
      ctx->Error("Unary operators should have no arguments. Invalid definition "
                 "of unary operator " +
                     ctx->highlightError(OpToString(opr)),
                 fileRange);
    }
  } else {
    if (arguments.size() != 1) {
      ctx->Error("Invalid number of arguments for Binary operator " +
                     ctx->highlightError(OpToString(opr)) +
                     ". Binary operators should have only 1 argument",
                 fileRange);
    }
  }
  Vec<IR::QatType *> generatedTypes;
  SHOW("Generating types")
  for (auto *arg : arguments) {
    if (arg->isTypeMember()) {
      if (coreType->hasMember(arg->getName())) {
        if (isVariationFn) {
          generatedTypes.push_back(coreType->getTypeOfMember(arg->getName()));
        } else {
          ctx->Error("This operator is not marked as a variation. It "
                     "cannot use the member argument syntax",
                     fileRange);
        }
      } else {
        ctx->Error("No non-static member named " + arg->getName() +
                       " in the core type " + coreType->getFullName(),
                   arg->getFileRange());
      }
    } else {
      generatedTypes.push_back(arg->getType()->emit(ctx));
    }
  }
  if (isUnaryOp(opr)) {
    if (coreType->hasUnaryOperator(OpToString(opr))) {
      ctx->Error("Unary operator " + ctx->highlightError(OpToString(opr)) +
                     " already exists for core type " +
                     ctx->highlightError(coreType->getFullName()),
                 fileRange);
    }
  } else {
    if (coreType->hasBinaryOperator(OpToString(opr), generatedTypes[0])) {
      ctx->Error("Binary operator " + ctx->highlightError(OpToString(opr)) +
                     " already exists for core type " +
                     ctx->highlightError(coreType->getFullName()) +
                     " with rhs type " +
                     ctx->highlightError(generatedTypes.front()->toString()),
                 fileRange);
    }
  }
  SHOW("Argument types generated")
  Vec<IR::Argument> args;
  SHOW("Setting variability of arguments")
  for (usize i = 0; i < generatedTypes.size(); i++) {
    if (arguments.at(i)->isTypeMember()) {
      args.push_back(IR::Argument::CreateMember(arguments.at(i)->getName(),
                                                generatedTypes.at(i), i));
    } else {
      args.push_back(arguments.at(i)->getType()->isVariable()
                         ? IR::Argument::CreateVariable(
                               arguments.at(i)->getName(),
                               arguments.at(i)->getType()->emit(ctx), i)
                         : IR::Argument::Create(arguments.at(i)->getName(),
                                                generatedTypes.at(i), i));
    }
  }
  SHOW("Variability setting complete")
  SHOW("About to create operator function")
  memberFn = IR::MemberFunction::CreateOperator(
      coreType, !isUnaryOp(opr), isVariationFn, OpToString(opr),
      returnType->emit(ctx), args, fileRange, ctx->getVisibInfo(kind),
      ctx->llctx);
}

IR::Value *OperatorPrototype::emit(IR::Context *ctx) { return memberFn; }

void OperatorPrototype::setCoreType(IR::CoreType *_coreType) const {
  coreType = _coreType;
}

Json OperatorPrototype::toJson() const {
  Vec<JsonValue> args;
  for (auto *arg : arguments) {
    auto aJson =
        Json()
            ._("name", arg->getName())
            ._("type", arg->getType() ? arg->getType()->toJson() : Json())
            ._("isMemberArg", arg->isTypeMember())
            ._("fileRange", arg->getFileRange());
    args.push_back(aJson);
  }
  return Json()
      ._("nodeType", "operatorPrototype")
      ._("isVariation", isVariationFn)
      ._("operator", OpToString(opr))
      ._("returnType", returnType->toJson())
      ._("arguments", args)
      ._("fileRange", fileRange);
}

OperatorDefinition::OperatorDefinition(OperatorPrototype *_prototype,
                                       Vec<Sentence *>    _sentences,
                                       utils::FileRange   _fileRange)
    : Node(std::move(_fileRange)), sentences(std::move(_sentences)),
      prototype(_prototype) {}

void OperatorDefinition::define(IR::Context *ctx) { prototype->define(ctx); }

IR::Value *OperatorDefinition::emit(IR::Context *ctx) {
  auto *fnEmit = (IR::MemberFunction *)prototype->emit(ctx);
  ctx->fn      = fnEmit;
  SHOW("Set active operator function: " << fnEmit->getFullName())
  auto *block = new IR::Block(fnEmit, nullptr);
  SHOW("Created entry block")
  block->setActive(ctx->builder);
  SHOW("Set new block as the active block")
  SHOW("About to allocate necessary arguments")
  auto  argIRTypes = fnEmit->getType()->asFunction()->getArgumentTypes();
  auto *corePtrTy  = argIRTypes.at(0)->getType()->asPointer();
  auto *self       = block->newValue("''", corePtrTy, prototype->isVariationFn);
  ctx->builder.CreateStore(fnEmit->getLLVMFunction()->getArg(0u),
                           self->getLLVM());
  ctx->selfVal =
      ctx->builder.CreateLoad(corePtrTy->getLLVMType(), self->getAlloca());
  for (usize i = 1; i < argIRTypes.size(); i++) {
    SHOW("Argument type is " << argIRTypes.at(i)->getType()->toString())
    if (argIRTypes.at(i)->isMemberArgument()) {
      auto *memPtr = ctx->builder.CreateStructGEP(
          corePtrTy->getSubType()->getLLVMType(), ctx->selfVal,
          corePtrTy->getSubType()
              ->asCore()
              ->getIndexOf(argIRTypes.at(i)->getName())
              .value());
      ctx->builder.CreateStore(fnEmit->getLLVMFunction()->getArg(i), memPtr,
                               false);
    } else {
      SHOW("Argument is variable")
      auto *argVal = block->newValue(argIRTypes.at(i)->getName(),
                                     argIRTypes.at(i)->getType(), true);
      SHOW("Created local value for the argument")
      ctx->builder.CreateStore(fnEmit->getLLVMFunction()->getArg(i),
                               argVal->getAlloca(), false);
    }
  }
  emitSentences(sentences, ctx);
  ctx->selfVal = nullptr;
  IR::functionReturnHandler(
      ctx, fnEmit, sentences.empty() ? fileRange : sentences.back()->fileRange);
  SHOW("Sentences emitted")
  return nullptr;
}

void OperatorDefinition::setCoreType(IR::CoreType *coreType) const {
  prototype->setCoreType(coreType);
}

Json OperatorDefinition::toJson() const {
  Vec<JsonValue> sntcs;
  for (auto *sentence : sentences) {
    sntcs.push_back(sentence->toJson());
  }
  return Json()
      ._("nodeType", "operatorDefinition")
      ._("prototype", prototype->toJson())
      ._("body", sntcs)
      ._("fileRange", fileRange);
}

} // namespace qat::ast