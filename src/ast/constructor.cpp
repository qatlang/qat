#include "./constructor.hpp"
#include "../show.hpp"
#include "llvm/IR/GlobalValue.h"
#include <algorithm>
#include <bits/ranges_algo.h>
#include <vector>

namespace qat::ast {

ConstructorPrototype::ConstructorPrototype(ConstructorType       _type,
                                           Vec<Argument *>       _arguments,
                                           utils::VisibilityKind _visibility,
                                           utils::FileRange      _fileRange)
    : Node(std::move(_fileRange)), arguments(std::move(_arguments)),
      visibility(_visibility), type(_type) {}

IR::Value *ConstructorPrototype::emit(IR::Context *ctx) {
  if (!coreType) {
    ctx->Error("No core type found for this constructor", fileRange);
  }
  IR::MemberFunction         *function;
  Vec<IR::QatType *>          generatedTypes;
  Vec<IR::CoreType::Member *> presentRefMembers;
  // FIXME - Check if member arguments are repeating
  SHOW("Generating types")
  for (auto *arg : arguments) {
    if (arg->isTypeMember()) {
      SHOW("Arg is type member")
      if (coreType->hasMember(arg->getName())) {
        auto *mem = coreType->getMember(arg->getName());
        SHOW("Getting core type member: " << arg->getName())
        if (mem->type->isReference()) {
          presentRefMembers.push_back(mem);
        }
        generatedTypes.push_back(mem->type);
      } else {
        ctx->Error("No non-static member named " + arg->getName() +
                       " in the core type " + coreType->getFullName(),
                   arg->getFileRange());
      }
    } else {
      generatedTypes.push_back(arg->getType()->emit(ctx));
    }
    SHOW("Generated type of the argument is "
         << generatedTypes.back()->toString())
  }
  SHOW("Argument types generated")
  if (coreType->hasConstructorWithTypes(generatedTypes)) {
    ctx->Error(
        "Another constructor with similar signature exists for the core type " +
            ctx->highlightError(coreType->getFullName()),
        fileRange);
  }
  auto                       &allMembers = coreType->getMembers();
  Vec<IR::CoreType::Member *> refMembers;
  for (auto *mem : allMembers) {
    if (mem->type->isReference()) {
      SHOW("Ref member name: " << mem->name)
      refMembers.push_back(mem);
    }
  }
  bool                        allRefsPresent = true;
  Vec<IR::CoreType::Member *> absentMembers;
  for (auto *mem : refMembers) {
    if (mem->type->isReference()) {
      SHOW("Member " << mem->name << " is a reference")
      bool memRes = false;
      for (auto *ref : presentRefMembers) {
        if (mem->name == ref->name) {
          memRes = true;
          break;
        }
      }
      if (!memRes) {
        SHOW("Ref member " << mem->name << " not present")
        absentMembers.push_back(mem);
        allRefsPresent = false;
      }
    }
  }
  if (!allRefsPresent) {
    SHOW("Absent member count: " << absentMembers.size())
    String message;
    for (usize i = 0; i < absentMembers.size(); i++) {
      SHOW("Absent member name is: " << absentMembers.at(i)->name)
      message += ctx->highlightError(absentMembers.at(i)->name);
      if (absentMembers.size() > 1) {
        if (i == (absentMembers.size() - 2)) {
          message += " and ";
        } else if (i != (absentMembers.size() - 1)) {
          message += ", ";
        }
      }
    }
    ctx->Error(String("Member") + (absentMembers.size() == 1 ? " " : "s ") +
                   message + " in the core type " +
                   ctx->highlightError(coreType->getFullName()) +
                   (absentMembers.size() == 1 ? " is a reference"
                                              : " are references") +
                   " and " +
                   (absentMembers.size() == 1 ? "is not" : "are not") +
                   " initialised in the "
                   "constructor. Please check logic and make "
                   "necessary changes",
               fileRange);
  }
  Vec<IR::Argument> args;
  SHOW("Setting variability of arguments")
  for (usize i = 0; i < generatedTypes.size(); i++) {
    if (arguments.at(i)->isTypeMember()) {
      SHOW("Creating member argument")
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
  SHOW("About to create function")
  function = IR::MemberFunction::CreateConstructor(
      coreType, args, false, fileRange, ctx->getVisibInfo(visibility),
      ctx->llctx);
  SHOW("Constructor created in the IR")
  // TODO - Set calling convention
  return function;
}

void ConstructorPrototype::setCoreType(IR::CoreType *_coreType) {
  coreType = _coreType;
}

nuo::Json ConstructorPrototype::toJson() const {
  Vec<nuo::JsonValue> args;
  for (auto *arg : arguments) {
    auto aJson =
        nuo::Json()
            ._("name", arg->getName())
            ._("type", arg->getType() ? arg->getType()->toJson() : nuo::Json())
            ._("isMemberArg", arg->isTypeMember())
            ._("fileRange", arg->getFileRange());
    args.push_back(aJson);
  }
  return nuo::Json()
      ._("nodeType", "constructorPrototype")
      ._("arguments", args)
      ._("visibility", utils::kindToJsonValue(visibility))
      ._("fileRange", fileRange);
}

ConstructorDefinition::ConstructorDefinition(ConstructorPrototype *_prototype,
                                             Vec<Sentence *>       _sentences,
                                             utils::FileRange      _fileRange)
    : Node(std::move(_fileRange)), sentences(std::move(_sentences)),
      prototype(_prototype) {}

void ConstructorDefinition::define(IR::Context *ctx) { prototype->define(ctx); }

IR::Value *ConstructorDefinition::emit(IR::Context *ctx) {
  auto *fnEmit = (IR::MemberFunction *)prototype->emit(ctx);
  ctx->fn      = fnEmit;
  SHOW("Set active contructor: " << fnEmit->getFullName())
  auto *block = new IR::Block(fnEmit, nullptr);
  SHOW("Created entry block")
  block->setActive(ctx->builder);
  SHOW("Set new block as the active block")
  SHOW("About to allocate necessary arguments")
  auto  argIRTypes = fnEmit->getType()->asFunction()->getArgumentTypes();
  auto *corePtrTy  = argIRTypes.at(0)->getType()->asPointer();
  // FIXME - Check if variability should be true
  auto *self = block->newValue("''", corePtrTy, true);
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

void ConstructorDefinition::setCoreType(IR::CoreType *coreType) const {
  prototype->setCoreType(coreType);
}

nuo::Json ConstructorDefinition::toJson() const {
  Vec<nuo::JsonValue> sntcs;
  for (auto *sentence : sentences) {
    sntcs.push_back(sentence->toJson());
  }
  return nuo::Json()
      ._("nodeType", "constructorDefinition")
      ._("prototype", prototype->toJson())
      ._("body", sntcs)
      ._("fileRange", fileRange);
}

} // namespace qat::ast