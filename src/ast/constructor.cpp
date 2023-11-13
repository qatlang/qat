#include "./constructor.hpp"
#include "../show.hpp"
#include "node.hpp"
#include "llvm/IR/GlobalValue.h"
#include <algorithm>
// #include <bits/ranges_algo.h>
#include <vector>

namespace qat::ast {

ConstructorPrototype::ConstructorPrototype(ConstructorType _type, FileRange _nameRange, Vec<Argument*> _arguments,
                                           Maybe<VisibilitySpec> _visibSpec, FileRange _fileRange,
                                           Maybe<Identifier> _argName)
    : Node(std::move(_fileRange)), arguments(std::move(_arguments)), visibSpec(_visibSpec), type(_type),
      argName(std::move(_argName)), nameRange(std::move(_nameRange)) {}

ConstructorPrototype::~ConstructorPrototype() {
  for (auto* arg : arguments) {
    delete arg;
  }
}

ConstructorPrototype* ConstructorPrototype::Normal(FileRange nameRange, Vec<Argument*> args,
                                                   Maybe<VisibilitySpec> visibSpec, FileRange fileRange) {
  return new ast::ConstructorPrototype(ConstructorType::normal, nameRange, std::move(args), visibSpec,
                                       std::move(fileRange));
}

ConstructorPrototype* ConstructorPrototype::Default(Maybe<VisibilitySpec> visibSpec, FileRange nameRange,
                                                    FileRange fileRange) {
  return new ast::ConstructorPrototype(ConstructorType::Default, nameRange, {}, visibSpec, std::move(fileRange));
}

ConstructorPrototype* ConstructorPrototype::Copy(Maybe<VisibilitySpec> visibSpec, FileRange nameRange,
                                                 FileRange fileRange, Identifier _argName) {
  return new ast::ConstructorPrototype(ConstructorType::copy, nameRange, {}, visibSpec, std::move(fileRange), _argName);
}

ConstructorPrototype* ConstructorPrototype::Move(Maybe<VisibilitySpec> visibSpec, FileRange nameRange,
                                                 FileRange fileRange, Identifier _argName) {
  return new ast::ConstructorPrototype(ConstructorType::move, nameRange, {}, visibSpec, std::move(fileRange), _argName);
}

IR::Value* ConstructorPrototype::emit(IR::Context* ctx) {
  if (!coreType) {
    ctx->Error("No core type found for this constructor", fileRange);
  }
  IR::MemberFunction* function = nullptr;
  if (type == ConstructorType::normal) {
    Vec<IR::QatType*>          generatedTypes;
    Vec<String>                presentMembers;
    Vec<IR::CoreType::Member*> presentRefMembers;
    SHOW("Generating types")
    for (auto* arg : arguments) {
      if (arg->isTypeMember()) {
        SHOW("Arg is type member")
        if (coreType->hasMember(arg->getName().value)) {
          for (auto mem : presentMembers) {
            if (mem == arg->getName().value) {
              ctx->Error("The member " + ctx->highlightError(mem) + " is repeating here", arg->getName().range);
            }
          }
          auto* mem = coreType->getMember(arg->getName().value);
          mem->addMention(arg->getName().range);
          presentMembers.push_back(arg->getName().value);
          SHOW("Getting core type member: " << arg->getName().value)
          if (mem->type->isReference()) {
            presentRefMembers.push_back(mem);
          }
          generatedTypes.push_back(mem->type);
        } else {
          ctx->Error("No non-static member named " + arg->getName().value + " in the core type " +
                         coreType->getFullName(),
                     arg->getName().range);
        }
      } else {
        generatedTypes.push_back(arg->getType()->emit(ctx));
      }
      SHOW("Generated type of the argument is " << generatedTypes.back()->toString())
    }
    SHOW("Argument types generated")
    if (coreType->hasConstructorWithTypes(generatedTypes)) {
      ctx->Error("Another constructor with similar signature exists for the "
                 "core type " +
                     ctx->highlightError(coreType->getFullName()),
                 fileRange);
    }
    auto&                      allMembers = coreType->getMembers();
    Vec<IR::CoreType::Member*> refMembers;
    for (auto* mem : allMembers) {
      if (mem->type->isReference()) {
        SHOW("Ref member name: " << mem->name.value)
        refMembers.push_back(mem);
      }
    }
    bool                       allRefsPresent = true;
    Vec<IR::CoreType::Member*> absentMembers;
    for (auto* mem : refMembers) {
      if (mem->type->isReference()) {
        SHOW("Member " << mem->name.value << " is a reference")
        bool memRes = false;
        for (auto* ref : presentRefMembers) {
          if (mem->name.value == ref->name.value) {
            memRes = true;
            break;
          }
        }
        if (!memRes) {
          SHOW("Ref member " << mem->name.value << " not present")
          absentMembers.push_back(mem);
          allRefsPresent = false;
        }
      }
    }
    if (!allRefsPresent) {
      SHOW("Absent member count: " << absentMembers.size())
      String message;
      for (usize i = 0; i < absentMembers.size(); i++) {
        SHOW("Absent member name is: " << absentMembers.at(i)->name.value)
        message += ctx->highlightError(absentMembers.at(i)->name.value);
        if (absentMembers.size() > 1) {
          if (i == (absentMembers.size() - 2)) {
            message += " and ";
          } else if (i != (absentMembers.size() - 1)) {
            message += ", ";
          }
        }
      }
      ctx->Error(String("Member") + (absentMembers.size() == 1 ? " " : "s ") + message + " in the core type " +
                     ctx->highlightError(coreType->getFullName()) +
                     (absentMembers.size() == 1 ? " is a reference" : " are references") + " and " +
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
        args.push_back(IR::Argument::CreateMember(arguments.at(i)->getName(), generatedTypes.at(i), i));
      } else {
        args.push_back(
            arguments.at(i)->isVariable()
                ? IR::Argument::CreateVariable(arguments.at(i)->getName(), arguments.at(i)->getType()->emit(ctx), i)
                : IR::Argument::Create(arguments.at(i)->getName(), generatedTypes.at(i), i));
      }
    }
    SHOW("About to create function")
    function = IR::MemberFunction::CreateConstructor(coreType, nameRange, args, false, fileRange,
                                                     ctx->getVisibInfo(visibSpec), ctx);
    SHOW("Constructor created in the IR")
    // TODO - Set calling convention
    return function;
  } else if (type == ConstructorType::Default) {
    for (auto* mem : coreType->getMembers()) {
      if (mem->type->isReference()) {
        ctx->Error("Core type " + ctx->highlightError(coreType->getFullName()) +
                       " has members of reference type, and hence cannot use a "
                       "default constructor",
                   fileRange);
      }
    }
    function =
        IR::MemberFunction::DefaultConstructor(coreType, nameRange, ctx->getVisibInfo(visibSpec), fileRange, ctx);
    return function;
  } else if (type == ConstructorType::copy) {
    function = IR::MemberFunction::CopyConstructor(coreType, nameRange, argName.value(), fileRange, ctx);
    return function;
  } else if (type == ConstructorType::move) {
    function = IR::MemberFunction::MoveConstructor(coreType, nameRange, argName.value(), fileRange, ctx);
    return function;
  }
  return function;
}

void ConstructorPrototype::setCoreType(IR::CoreType* _coreType) { coreType = _coreType; }

Json ConstructorPrototype::toJson() const {
  Vec<JsonValue> args;
  for (auto* arg : arguments) {
    auto aJson = Json()
                     ._("name", arg->getName())
                     ._("type", arg->getType() ? arg->getType()->toJson() : Json())
                     ._("isMemberArg", arg->isTypeMember());
    args.push_back(aJson);
  }
  return Json()
      ._("nodeType", "constructorPrototype")
      ._("arguments", args)
      ._("hasVisibility", visibSpec.has_value())
      ._("visibility", visibSpec.has_value() ? visibSpec->toJson() : JsonValue())
      ._("fileRange", fileRange);
}

ConstructorDefinition::ConstructorDefinition(ConstructorPrototype* _prototype, Vec<Sentence*> _sentences,
                                             FileRange _fileRange)
    : Node(std::move(_fileRange)), sentences(std::move(_sentences)), prototype(_prototype) {}

void ConstructorDefinition::define(IR::Context* ctx) { prototype->define(ctx); }

IR::Value* ConstructorDefinition::emit(IR::Context* ctx) {
  auto* fnEmit = (IR::MemberFunction*)prototype->emit(ctx);
  auto* oldFn  = ctx->setActiveFunction(fnEmit);
  SHOW("Set active contructor: " << fnEmit->getFullName())
  auto* block = new IR::Block(fnEmit, nullptr);
  SHOW("Created entry block")
  block->setActive(ctx->builder);
  SHOW("Set new block as the active block")
  SHOW("About to allocate necessary arguments")
  auto  argIRTypes = fnEmit->getType()->asFunction()->getArgumentTypes();
  auto* coreRefTy  = argIRTypes.at(0)->getType()->asReference();
  auto* self       = block->newValue("''", coreRefTy, true, coreRefTy->getSubType()->asCore()->getName().range);
  ctx->builder.CreateStore(fnEmit->getLLVMFunction()->getArg(0u), self->getLLVM());
  self->loadImplicitPointer(ctx->builder);
  if ((prototype->type == ConstructorType::copy) || (prototype->type == ConstructorType::move)) {
    auto* argVal =
        block->newValue(prototype->argName->value,
                        IR::ReferenceType::get(prototype->type == ConstructorType::move, prototype->coreType, ctx),
                        false, prototype->argName->range);
    ctx->builder.CreateStore(fnEmit->getLLVMFunction()->getArg(1), argVal->getAlloca(), false);
    argVal->loadImplicitPointer(ctx->builder);
  } else {
    for (usize i = 1; i < argIRTypes.size(); i++) {
      SHOW("Argument type is " << argIRTypes.at(i)->getType()->toString())
      if (argIRTypes.at(i)->isMemberArgument()) {
        auto* memPtr = ctx->builder.CreateStructGEP(
            coreRefTy->getSubType()->getLLVMType(), self->getLLVM(),
            coreRefTy->getSubType()->asCore()->getIndexOf(argIRTypes.at(i)->getName()).value());
        ctx->builder.CreateStore(fnEmit->getLLVMFunction()->getArg(i), memPtr, false);
      } else {
        SHOW("Argument is variable")
        auto* argVal = block->newValue(argIRTypes.at(i)->getName(), argIRTypes.at(i)->getType(),
                                       prototype->arguments.at(i - 1)->isVariable(),
                                       prototype->arguments.at(i - 1)->getName().range);
        SHOW("Created local value for the argument")
        ctx->builder.CreateStore(fnEmit->getLLVMFunction()->getArg(i), argVal->getAlloca(), false);
      }
    }
  }
  emitSentences(sentences, ctx);
  IR::functionReturnHandler(ctx, fnEmit, sentences.empty() ? fileRange : sentences.back()->fileRange);
  SHOW("Sentences emitted")
  (void)ctx->setActiveFunction(oldFn);
  return nullptr;
}

void ConstructorDefinition::setCoreType(IR::CoreType* coreType) const { prototype->setCoreType(coreType); }

Json ConstructorDefinition::toJson() const {
  Vec<JsonValue> sntcs;
  for (auto* sentence : sentences) {
    sntcs.push_back(sentence->toJson());
  }
  return Json()
      ._("nodeType", "constructorDefinition")
      ._("prototype", prototype->toJson())
      ._("body", sntcs)
      ._("fileRange", fileRange);
}

} // namespace qat::ast