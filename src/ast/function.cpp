#include "./function.hpp"
#include "../IR/logic.hpp"
#include "../IR/types/future.hpp"
#include "../show.hpp"
#include "sentence.hpp"
#include "types/generic_abstract.hpp"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"

#define MainFirstArgBitwidth 32u

namespace qat::ast {

FunctionPrototype::FunctionPrototype(Identifier _name, Vec<Argument*> _arguments, bool _isVariadic,
                                     QatType* _returnType, bool _is_async, llvm::GlobalValue::LinkageTypes _linkageType,
                                     String _callingConv, utils::VisibilityKind _visibility,
                                     const FileRange& _fileRange, Vec<ast::GenericAbstractType*> _generics)
    : Node(_fileRange), name(std::move(_name)), isAsync(_is_async), arguments(std::move(_arguments)),
      isVariadic(_isVariadic), returnType(_returnType), callingConv(std::move(_callingConv)), visibility(_visibility),
      generics(std::move(_generics)), linkageType(_linkageType) {}

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
  SHOW("Creating function " << name.value << " with generic count: " << generics.size())
  ctx->nameCheckInModule(name, isGeneric() ? "generic function" : "function",
                         isGeneric() ? Maybe<String>(genericFn->getID()) : None);
  Vec<IR::QatType*> generatedTypes;
  bool              isMainFn = false;
  String            fnName   = name.value;
  SHOW("Creating function")
  if (genericFn) {
    fnName = variantName.value();
  }
  if ((fnName == "main") && (mod->getFullNameWithChild("main") == "main")) {
    SHOW("Is main function")
    linkageType = llvm::GlobalValue::LinkageTypes::LinkOnceAnyLinkage;
    isMainFn    = true;
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
                           ctx->highlightError("multiptr[cstring ?]") + " type",
                       arguments.at(0)->getType()->fileRange);
          }
          mod->setHasMainFn();
          ctx->hasMain = true;
        } else {
          ctx->Error("Type of the first argument of the " + ctx->highlightError("main") + " function should be " +
                         ctx->highlightError("multiptr[+ cstring ?]"),
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
              IR::UnsignedType::get(32u, ctx->llctx), 0u));
      args.push_back(IR::Argument::Create(
          Identifier(arguments.at(0)->getName().value + "'data", arguments.at(0)->getName().range),
          IR::PointerType::get(false, IR::CType::getCString(ctx), IR::PointerOwner::OfAnonymous(), false, ctx), 1u));
    }
  } else {
    for (usize i = 0; i < generatedTypes.size(); i++) {
      args.push_back(
          arguments.at(i)->getType()->isVariable()
              ? IR::Argument::CreateVariable(arguments.at(i)->getName(), arguments.at(i)->getType()->emit(ctx), i)
              : IR::Argument::Create(arguments.at(i)->getName(), generatedTypes.at(i), i));
    }
  }
  SHOW("Variability setting complete")
  auto* retTy = returnType->emit(ctx);
  if (isAsync) {
    if (!retTy->isFuture()) {
      ctx->Error("An async function should always return a " + ctx->highlightError("future"), fileRange);
    }
  }
  if (isGeneric()) {
    Vec<IR::GenericType*> genericTypes;
    for (auto* gen : generics) {
      genericTypes.push_back(gen->toIRGenericType());
    }
    SHOW("About to create generic function")
    auto* fun = IR::Function::Create(mod, Identifier(fnName, name.range), std::move(genericTypes), retTy, isAsync, args,
                                     isVariadic, fileRange, ctx->getVisibInfo(visibility), ctx->llctx);
    SHOW("Created IR function")
    return fun;
  } else {
    SHOW("About to create function")
    auto* fun = mod->createFunction(Identifier(fnName, name.range), retTy, isAsync, args, isVariadic, fileRange,
                                    ctx->getVisibInfo(visibility), linkageType, ctx->llctx);
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
      ._("isAsync", isAsync)
      ._("returnType", returnType->toJson())
      ._("arguments", args)
      ._("isVariadic", isVariadic)
      ._("callingConvention", callingConv);
}

FunctionDefinition::FunctionDefinition(FunctionPrototype* _prototype, Vec<Sentence*> _sentences, FileRange _fileRange)
    : Node(std::move(_fileRange)), sentences(std::move(_sentences)), prototype(_prototype) {}

void FunctionDefinition::define(IR::Context* ctx) {
  if (!prototype->isGeneric()) {
    prototype->define(ctx);
  } else {
    auto* tempFn = new IR::GenericFunction(prototype->name, prototype->generics, this, ctx->getMod(),
                                           ctx->getVisibInfo(prototype->visibility));
    for (auto* gen : prototype->generics) {
      gen->emit(ctx);
    }
    prototype->genericFn = tempFn;
  }
}

bool FunctionDefinition::isGeneric() const { return prototype->isGeneric(); }

IR::Value* FunctionDefinition::emit(IR::Context* ctx) {
  if (prototype->isGeneric()) {
    SHOW("Function is generic")
    (void)prototype->emit(ctx);
  }
  SHOW("Getting IR function from prototype")
  auto* fnEmit = prototype->function;
  ctx->fn      = fnEmit;
  SHOW("Set active function: " << fnEmit->getFullName())
  if (prototype->isAsync) {
    auto* fun        = fnEmit->getAsyncSubFunction();
    auto* entryBlock = llvm::BasicBlock::Create(ctx->llctx, "entry", fnEmit->getLLVMFunction());
    ctx->builder.SetInsertPoint(entryBlock);
    SHOW("Getting void pointer")
    auto* voidPtrTy = llvm::Type::getInt8Ty(ctx->llctx)->getPointerTo();
    ctx->getMod()->linkNative(IR::NativeUnit::pthreadCreate);
    ctx->getMod()->linkNative(IR::NativeUnit::pthreadAttrInit);
    SHOW("Linked pthread_create & pthread_attr_init functions to current module")
    auto* pthreadCreate     = ctx->mod->getLLVMModule()->getFunction("pthread_create");
    auto* pthreadAttrInitFn = ctx->mod->getLLVMModule()->getFunction("pthread_attr_init");
    auto* argAlloca         = ctx->builder.CreateAlloca(fnEmit->getAsyncArgType());
    auto* pthreadAttrAlloca = ctx->builder.CreateAlloca(llvm::StructType::getTypeByName(ctx->llctx, "pthread_attr_t"));
    SHOW("Create allocas for async arg and pthread_attr_t")
    const auto& fnOrigArgs = fnEmit->getType()->asFunction()->getArgumentTypes();
    for (usize i = 0; i < fnOrigArgs.size(); i++) {
      auto* argSubGEP = ctx->builder.CreateStructGEP(argAlloca->getAllocatedType(), argAlloca, i);
      ctx->builder.CreateStore(fnEmit->getLLVMFunction()->getArg(i), argSubGEP);
    }
    ctx->builder.CreateCall(pthreadAttrInitFn->getFunctionType(), pthreadAttrInitFn, {pthreadAttrAlloca});
    auto* futureTy = fnEmit->getType()->asFunction()->getReturnArgType()->asReference()->getSubType()->asFuture();
    SHOW("Getting pthread pointer")
    auto* futureRefRef = ctx->builder.CreateStructGEP(argAlloca->getAllocatedType(), argAlloca, fnOrigArgs.size() - 1);
    auto* pthreadPtr   = ctx->builder.CreateStructGEP(
        futureTy->getLLVMType(), ctx->builder.CreateLoad(futureTy->getLLVMType()->getPointerTo(), futureRefRef), 0);
    SHOW("Calling pthread_create")
    ctx->builder.CreateCall(pthreadCreate->getFunctionType(), pthreadCreate,
                            {pthreadPtr, pthreadAttrAlloca, fun, ctx->builder.CreatePointerCast(argAlloca, voidPtrTy)});
    {
      SHOW("Loading future from async arg")
      auto* futureRef = ctx->builder.CreateLoad(futureTy->getLLVMType()->getPointerTo(), futureRefRef);
      SHOW("Future loaded from async arg")
      ctx->getMod()->linkNative(IR::NativeUnit::malloc);
      auto* mallocFn = ctx->getMod()->getLLVMModule()->getFunction("malloc");
      ctx->builder.CreateStore(
          ctx->builder.CreatePointerCast(
              ctx->builder.CreateCall(
                  mallocFn->getFunctionType(), mallocFn,
                  {llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx),
                                          ctx->getMod()->getLLVMModule()->getDataLayout().getTypeAllocSize(
                                              llvm::Type::getInt1Ty(ctx->llctx)))}),
              llvm::Type::getInt1Ty(ctx->llctx)->getPointerTo()),
          ctx->builder.CreateStructGEP(futureTy->getLLVMType(), futureRef, 2u));
      ctx->builder.CreateStore(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 0u),
          ctx->builder.CreateLoad(llvm::Type::getInt1Ty(ctx->llctx)->getPointerTo(),
                                  ctx->builder.CreateStructGEP(futureTy->getLLVMType(), futureRef, 2u)));
      SHOW("Future: Stored tag")
      if (!futureTy->getSubType()->isVoid()) {
        ctx->builder.CreateStore(
            ctx->builder.CreatePointerCast(
                ctx->builder.CreateCall(
                    mallocFn->getFunctionType(), mallocFn,
                    {llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx),
                                            ctx->getMod()->getLLVMModule()->getDataLayout().getTypeAllocSize(
                                                futureTy->getSubType()->getLLVMType()))}),
                futureTy->getSubType()->getLLVMType()->getPointerTo()),
            ctx->builder.CreateStructGEP(futureTy->getLLVMType(), futureRef, 3u));
        ctx->builder.CreateStore(
            llvm::Constant::getNullValue(futureTy->getSubType()->getLLVMType()),
            ctx->builder.CreateLoad(futureTy->getSubType()->getLLVMType()->getPointerTo(),
                                    ctx->builder.CreateStructGEP(futureTy->getLLVMType(), futureRef, 3u)));
        SHOW("Future: Stored null value")
      }
    }
    ctx->builder.CreateRetVoid();
    SHOW("Switching to the internal async function")
    auto* block = new IR::Block(fnEmit, nullptr);
    SHOW("Created entry block for async fn")
    block->setActive(ctx->builder);
    SHOW("Async fn block set active")
    auto* asyncArgTy     = fnEmit->getAsyncArgType();
    auto* asyncArgAlloca = IR::Logic::newAlloca(fnEmit, "qat'asyncArgPtr", asyncArgTy->getPointerTo());
    ctx->builder.CreateStore(
        ctx->builder.CreatePointerCast(fnEmit->getAsyncSubFunction()->getArg(0), asyncArgTy->getPointerTo()),
        asyncArgAlloca);
    auto* asyncArg = ctx->builder.CreateLoad(asyncArgTy->getPointerTo(), asyncArgAlloca);
    for (usize i = 0; i < fnOrigArgs.size(); i++) {
      IR::LocalValue* localArg = nullptr;
      SHOW("Creating arg at " << i << " for async fn")
      if (i == (fnOrigArgs.size() - 1)) {
        localArg = block->newValue("qat'future", fnOrigArgs.at(i)->getType(), fnOrigArgs.at(i)->isVariable(),
                                   prototype->name.range);
      } else {
        localArg = block->newValue(fnOrigArgs.at(i)->getName(), fnOrigArgs.at(i)->getType(),
                                   fnOrigArgs.at(i)->isVariable(), prototype->arguments.at(i)->getName().range);
      };
      SHOW("Storing arg for async fn")
      SHOW("Arg alloca for future is: " << localArg->getType()->asReference()->toString())
      auto* argValLoad = ctx->builder.CreateLoad(fnOrigArgs.at(i)->getType()->getLLVMType(),
                                                 ctx->builder.CreateStructGEP(fnEmit->getAsyncArgType(), asyncArg, i));
      SHOW("Arg val loaded")
      ctx->builder.CreateStore(argValLoad, localArg->getAlloca());
      SHOW("Arg val stored")
    }
  } else {
    auto* block = new IR::Block(fnEmit, nullptr);
    SHOW("Created entry block")
    block->setActive(ctx->builder);
    SHOW("Set new block as the active block")
    bool isMainFn = fnEmit->getFullName() == "main";
    if (isMainFn) {
      SHOW("Calling module initialisers")
      auto modInits = IR::QatModule::collectModuleInitialisers();
      for (auto* modFn : modInits) {
        (void)modFn->call(ctx, {}, ctx->getMod());
      }
      SHOW("Storing args for main function")
      if (fnEmit->getType()->asFunction()->getArgumentCount() == 2u) {
        auto* cmdArgsVal = block->newValue(
            fnEmit->argumentNameAt(0).value.substr(0, fnEmit->argumentNameAt(0).value.find('\'')),
            IR::PointerType::get(false, IR::CType::getCString(ctx), IR::PointerOwner::OfAnonymous(), true, ctx), false,
            fnEmit->argumentNameAt(0).range);
        SHOW("Storing argument pointer")
        ctx->builder.CreateStore(
            fnEmit->getLLVMFunction()->getArg(1u),
            ctx->builder.CreateStructGEP(cmdArgsVal->getType()->getLLVMType(), cmdArgsVal->getLLVM(), 0u));
        SHOW("Storing argument count")
        ctx->builder.CreateStore(
            ctx->builder.CreateIntCast(fnEmit->getLLVMFunction()->getArg(0u), llvm::Type::getInt64Ty(ctx->llctx), true),
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
  }
  SHOW("Emitting sentences")
  emitSentences(sentences, ctx);
  SHOW("Sentences emitted")
  IR::functionReturnHandler(ctx, fnEmit, fileRange);
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
