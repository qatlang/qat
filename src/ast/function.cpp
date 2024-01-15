#include "./function.hpp"
#include "../IR/types/void.hpp"
#include "../show.hpp"
#include "sentence.hpp"
#include "types/generic_abstract.hpp"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"

#define MainFirstArgBitwidth 32u

namespace qat::ast {

FunctionPrototype::FunctionPrototype(Identifier _name, Vec<Argument*> _arguments, bool _isVariadic,
                                     Maybe<QatType*> _returnType, Maybe<PrerunExpression*> _checker,
                                     Maybe<PrerunExpression*> _genericConstraint, Maybe<MetaInfo> _metaInfo,
                                     Maybe<VisibilitySpec> _visibSpec, const FileRange& _fileRange,
                                     Vec<ast::GenericAbstractType*> _generics)
    : Node(_fileRange), name(std::move(_name)), arguments(std::move(_arguments)), isVariadic(_isVariadic),
      returnType(_returnType), metaInfo(std::move(_metaInfo)), visibSpec(_visibSpec), checker(_checker),
      genericConstraint(_genericConstraint), generics(std::move(_generics)) {}

FunctionPrototype::~FunctionPrototype() {
  for (auto* arg : arguments) {
    delete arg;
  }
  for (auto* gen : generics) {
    delete gen;
  }
}

bool FunctionPrototype::isGeneric() const { return !generics.empty(); }

Vec<GenericAbstractType*> FunctionPrototype::getGenerics() const { return generics; }

IR::Function* FunctionPrototype::createFunction(IR::Context* ctx) const {
  auto* mod = ctx->getMod();
  if (checker.has_value()) {
    auto cond = checker.value()->emit(ctx);
    if (cond->getType()->isBool()) {
      if (!llvm::cast<llvm::ConstantInt>(cond->getLLVMConstant())->getValue().getBoolValue()) {
        checkResult = false;
        return nullptr;
      }
    } else {
      ctx->Error("The condition for defining a function should be of " + ctx->highlightError("bool") + " type",
                 checker.value()->fileRange);
    }
  }
  SHOW("Creating function " << name.value << " with generic count: " << generics.size())
  ctx->nameCheckInModule(name, isGeneric() ? "generic function" : "function",
                         isGeneric() ? Maybe<String>(genericFn->getID()) : None);
  Vec<IR::QatType*> generatedTypes;
  String            fnName = name.value;
  SHOW("Creating function")
  if (genericFn) {
    fnName = variantName.value();
  }
  if ((fnName == "main") && (ctx->getMod()->getFullName() == "")) {
    SHOW("Is main function")
    linkageType = llvm::GlobalValue::LinkageTypes::LinkOnceAnyLinkage;
    isMainFn    = true;
  } else if (fnName == "main") {
    ctx->Error("Main function cannot be inside a named module", name.range);
  }
  SHOW("Generating types")
  for (auto* arg : arguments) {
    if (arg->isTypeMember()) {
      ctx->Error("Function is not a member function of a core type and cannot "
                 "use member argument syntax",
                 arg->getName().range);
    }
    auto* genType = arg->getType()->emit(ctx);
    generatedTypes.push_back(genType);
  }
  SHOW("Types generated")
  if (isMainFn) {
    if (visibSpec.has_value() && visibSpec->kind != VisibilityKind::pub) {
      ctx->Error("This is the " + ctx->highlightError("main") + " function, please make this function public like " +
                     ctx->highlightError("pub main"),
                 name.range);
    }
    if (ctx->getMod()->hasMainFn()) {
      ctx->Error(ctx->highlightError("main") + " function already exists in this module. Please check "
                                               "the codebase",
                 fileRange);
    } else {
      if (generatedTypes.size() == 1) {
        if (generatedTypes.at(0)->isPointer() && generatedTypes.at(0)->asPointer()->isMulti() &&
            generatedTypes.at(0)->asPointer()->getOwner().isAnonymous()) {
          if (generatedTypes.at(0)->asPointer()->isSubtypeVariable()) {
            ctx->Error("Type of the first argument of the " + ctx->highlightError("main") +
                           " function, cannot be a pointer with variability. "
                           "It should be of " +
                           ctx->highlightError("multiptr:[cstring ?]") + " type",
                       arguments.at(0)->getType()->fileRange);
          }
          mod->setHasMainFn();
          ctx->hasMain = true;
        } else {
          ctx->Error("Type of the first argument of the " + ctx->highlightError("main") + " function should be " +
                         ctx->highlightError("multiptr:[cstring ?]"),
                     arguments.at(0)->getType()->fileRange);
        }
      } else if (generatedTypes.empty()) {
        mod->setHasMainFn();
        ctx->hasMain = true;
      } else {
        ctx->Error("Main function can either have one argument, or none at "
                   "all. Please check the codebase and make necessary changes",
                   fileRange);
      }
    }
  }
  Vec<IR::Argument> args;
  SHOW("Setting variability of arguments")
  if (isMainFn) {
    if (!arguments.empty()) {
      args.push_back(
          // NOLINTNEXTLINE(readability-magic-numbers)
          IR::Argument::Create(
              Identifier(arguments.at(0)->getName().value + "'count", arguments.at(0)->getName().range),
              IR::UnsignedType::get(32u, ctx), 0u));
      args.push_back(IR::Argument::Create(
          Identifier(arguments.at(0)->getName().value + "'data", arguments.at(0)->getName().range),
          IR::PointerType::get(false, IR::CType::getCString(ctx), true, IR::PointerOwner::OfAnonymous(), false, ctx),
          1u));
    }
  } else {
    for (usize i = 0; i < generatedTypes.size(); i++) {
      args.push_back(
          arguments.at(i)->isVariable()
              ? IR::Argument::CreateVariable(arguments.at(i)->getName(), arguments.at(i)->getType()->emit(ctx), i)
              : IR::Argument::Create(arguments.at(i)->getName(), generatedTypes.at(i), i));
    }
  }
  SHOW("Variability setting complete")
  auto* retTy = returnType.has_value() ? returnType.value()->emit(ctx) : IR::VoidType::get(ctx->llctx);
  if (isMainFn) {
    if (!(retTy->isInteger() && (retTy->asInteger()->getBitwidth() == 32u))) {
      ctx->Error(ctx->highlightError("main") + " function expects to have a given type of " +
                     ctx->highlightError("i32"),
                 fileRange);
    }
  }
  Maybe<IR::MetaInfo> irMetaInfo;
  if (metaInfo.has_value()) {
    irMetaInfo = metaInfo.value().toIR(ctx);
  }
  if (isGeneric()) {
    Vec<IR::GenericParameter*> genericTypes;
    for (auto* gen : generics) {
      genericTypes.push_back(gen->toIRGenericType());
    }
    SHOW("About to create generic function")
    auto* fun = IR::Function::Create(mod, Identifier(fnName, name.range), None, std::move(genericTypes),
                                     IR::ReturnType::get(retTy), args, isVariadic, fileRange,
                                     ctx->getVisibInfo(visibSpec), ctx, None, irMetaInfo);
    SHOW("Created IR function")
    return fun;
  } else {
    SHOW("About to create function")
    auto* fun = IR::Function::Create(
        mod, Identifier(fnName, name.range),
        isMainFn
            ? Maybe<LinkNames>(LinkNames({LinkNameUnit(ctx->clangTargetInfo->getTriple().isWasm() ? "_start" : "main",
                                                       LinkUnitType::function)},
                                         "C", nullptr))
            : None,
        {}, IR::ReturnType::get(retTy), args, isVariadic, fileRange, ctx->getVisibInfo(visibSpec), ctx, None,
        irMetaInfo);
    SHOW("Created IR function")
    return fun;
  }
}

void FunctionPrototype::setVariantName(const String& value) const { variantName = value; }

void FunctionPrototype::unsetVariantName() const { variantName = None; }

void FunctionPrototype::define(IR::Context* ctx) {
  if (!isGeneric()) {
    function = createFunction(ctx);
  }
}

// NOLINTNEXTLINE(misc-unused-parameters)
IR::Value* FunctionPrototype::emit(IR::Context* ctx) {
  if (!isGeneric()) {
    return function;
  } else {
    function = createFunction(ctx);
    return function;
  }
}

Json FunctionPrototype::toJson() const {
  Vec<JsonValue> args;
  for (auto* arg : arguments) {
    auto aJson = Json()._("name", arg->getName())._("type", arg->getType() ? arg->getType()->toJson() : Json());
    args.push_back(aJson);
  }
  return Json()
      ._("nodeType", "functionPrototype")
      ._("name", name)
      ._("hasReturnType", returnType.has_value())
      ._("returnType", returnType.has_value() ? returnType.value()->toJson() : JsonValue())
      ._("arguments", args)
      ._("isVariadic", isVariadic)
      ._("hasVisibility", visibSpec.has_value())
      ._("visibility", visibSpec.has_value() ? visibSpec->toJson() : JsonValue())
      ._("hasMetaInfo", metaInfo.has_value())
      ._("metaInfo", metaInfo.has_value() ? metaInfo.value().toJson() : JsonValue());
}

FunctionDefinition::FunctionDefinition(FunctionPrototype* _prototype, Vec<Sentence*> _sentences, FileRange _fileRange)
    : Node(std::move(_fileRange)), sentences(std::move(_sentences)), prototype(_prototype) {}

void FunctionDefinition::define(IR::Context* ctx) {
  if (!prototype->isGeneric()) {
    prototype->hasDefinition = true;
    prototype->define(ctx);
  } else {
    if (prototype->checker.has_value()) {
      auto condRes = prototype->checker.value()->emit(ctx);
      if (condRes->getType()->isBool()) {
        prototype->checkResult = llvm::cast<llvm::ConstantInt>(condRes->getLLVMConstant())->getValue().getBoolValue();
        if (!prototype->checkResult.value()) {
          return;
        }
      } else {
        ctx->Error("The condition for defining this function should be of " + ctx->highlightError("bool") + " type",
                   prototype->checker.value()->fileRange);
      }
    }
    auto* tempFn = new IR::GenericFunction(prototype->name, prototype->generics, prototype->genericConstraint, this,
                                           ctx->getMod(), ctx->getVisibInfo(prototype->visibSpec));
    for (auto* gen : prototype->generics) {
      gen->emit(ctx);
    }
    prototype->genericFn = tempFn;
  }
}

bool FunctionDefinition::isGeneric() const { return prototype->isGeneric(); }

IR::Value* FunctionDefinition::emit(IR::Context* ctx) {
  if (prototype->isGeneric()) {
    prototype->hasDefinition = true;
    SHOW("Function is generic")
    (void)prototype->emit(ctx);
    prototype->hasDefinition = false;
  }
  if (prototype->checkResult.has_value() && !prototype->checkResult.value()) {
    return nullptr;
  }
  SHOW("Getting IR function from prototype")
  auto* fnEmit = prototype->function;
  auto* oldFn  = ctx->setActiveFunction(fnEmit);
  SHOW("Set active function: " << fnEmit->getFullName())
  auto* block = new IR::Block(fnEmit, nullptr);
  SHOW("Created entry block")
  block->setActive(ctx->builder);
  SHOW("Set new block as the active block")
  if (prototype->isMainFn) {
    SHOW("Calling module initialisers")
    auto modInits = IR::QatModule::collectModuleInitialisers();
    for (auto* modFn : modInits) {
      (void)modFn->call(ctx, {}, None, ctx->getMod());
    }
    SHOW("Storing args for main function")
    if (fnEmit->getType()->asFunction()->getArgumentCount() == 2u) {
      auto* cmdArgsVal = block->newValue(
          fnEmit->argumentNameAt(0).value.substr(0, fnEmit->argumentNameAt(0).value.find('\'')),
          IR::PointerType::get(false, IR::CType::getCString(ctx), false, IR::PointerOwner::OfAnonymous(), true, ctx),
          false, fnEmit->argumentNameAt(0).range);
      SHOW("Storing argument pointer")
      ctx->builder.CreateStore(
          fnEmit->getLLVMFunction()->getArg(1u),
          ctx->builder.CreateStructGEP(cmdArgsVal->getType()->getLLVMType(), cmdArgsVal->getLLVM(), 0u));
      SHOW("Storing argument count")
      ctx->builder.CreateStore(
          ctx->builder.CreateIntCast(fnEmit->getLLVMFunction()->getArg(0u), llvm::Type::getInt64Ty(ctx->llctx), false),
          ctx->builder.CreateStructGEP(cmdArgsVal->getType()->getLLVMType(), cmdArgsVal->getLLVM(), 1u));
    }
  } else {
    SHOW("About to allocate necessary arguments")
    auto argIRTypes = fnEmit->getType()->asFunction()->getArgumentTypes();
    SHOW("Iteration run for function is: " << fnEmit->getName().value)
    for (usize i = 0; i < argIRTypes.size(); i++) {
      SHOW("Argument name is " << argIRTypes.at(i)->getName())
      SHOW("Argument type is " << argIRTypes.at(i)->getType()->toString())
      auto* argVal = block->newValue(argIRTypes.at(i)->getName(), argIRTypes.at(i)->getType(),
                                     argIRTypes.at(i)->isVariable(), prototype->arguments.at(i)->getName().range);
      SHOW("Created local value for the argument")
      ctx->builder.CreateStore(fnEmit->getLLVMFunction()->getArg(i), argVal->getAlloca(), false);
    }
  }
  SHOW("Emitting sentences")
  emitSentences(sentences, ctx);
  SHOW("Sentences emitted")
  IR::functionReturnHandler(ctx, fnEmit, fileRange);
  (void)ctx->setActiveFunction(oldFn);
  return fnEmit;
}

Json FunctionDefinition::toJson() const {
  Vec<JsonValue> sntcs;
  for (auto* sentence : sentences) {
    sntcs.push_back(sentence->toJson());
  }
  return Json()
      ._("nodeType", "functionDefinition")
      ._("prototype", prototype->toJson())
      ._("body", sntcs)
      ._("fileRange", fileRange);
}

} // namespace qat::ast
