#include "./function.hpp"
#include "../IR/logic.hpp"
#include "../IR/types/future.hpp"
#include "../show.hpp"
#include "sentence.hpp"
#include "types/templated.hpp"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"

#define MainFirstArgBitwidth 32u

namespace qat::ast {

FunctionPrototype::FunctionPrototype(String _name, Vec<Argument*> _arguments, bool _isVariadic, QatType* _returnType,
                                     bool _is_async, llvm::GlobalValue::LinkageTypes _linkageType, String _callingConv,
                                     utils::VisibilityKind _visibility, const utils::FileRange& _fileRange,
                                     Vec<ast::TemplatedType*> _templates)
    : Node(_fileRange), name(std::move(_name)), isAsync(_is_async), arguments(std::move(_arguments)),
      isVariadic(_isVariadic), returnType(_returnType), callingConv(std::move(_callingConv)), visibility(_visibility),
      templates(std::move(_templates)), linkageType(_linkageType) {}

FunctionPrototype::~FunctionPrototype() {
  for (auto* arg : arguments) {
    delete arg;
  }
}

FunctionPrototype::FunctionPrototype(const FunctionPrototype& ref)
    : Node(ref.fileRange), name(ref.name), isAsync(ref.isAsync), arguments(ref.arguments), isVariadic(ref.isVariadic),
      returnType(ref.returnType), callingConv(ref.callingConv), visibility(ref.visibility), templates(ref.templates),
      linkageType(ref.linkageType), function(ref.function) {}

bool FunctionPrototype::isTemplate() const { return !templates.empty(); }

Vec<TemplatedType*> FunctionPrototype::getTemplates() const { return templates; }

IR::Function* FunctionPrototype::createFunction(IR::Context* ctx) const {
  Vec<IR::QatType*> generatedTypes;
  auto*             mod      = ctx->getMod();
  bool              isMainFn = false;
  String            fnName   = name;
  SHOW("Creating function")
  if (templateFn) {
    fnName = variantName.value();
  }
  SHOW("Checking function exists or not")
  if (mod->hasFunction(fnName)) {
    ctx->Error("A function named " + fnName + " exists already in this scope", fileRange);
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
                 arg->getFileRange());
    }
    auto* genType = arg->getType()->emit(ctx);
    generatedTypes.push_back(genType);
  }
  SHOW("Types generated")
  Vec<IR::Argument> args;
  SHOW("Setting variability of arguments")
  for (usize i = 0; i < generatedTypes.size(); i++) {
    args.push_back(
        arguments.at(i)->getType()->isVariable()
            ? IR::Argument::CreateVariable(arguments.at(i)->getName(), arguments.at(i)->getType()->emit(ctx), i)
            : IR::Argument::Create(arguments.at(i)->getName(), generatedTypes.at(i), i));
  }
  SHOW("Variability setting complete")
  auto* retTy = returnType->emit(ctx);
  if (isAsync) {
    if (!retTy->isFuture()) {
      ctx->Error("An async function should always return a future", fileRange);
    }
  }
  SHOW("About to create function")
  auto* fun = mod->createFunction(fnName, retTy, returnType->isVariable(), isAsync, args, isVariadic, fileRange,
                                  ctx->getVisibInfo(visibility), linkageType, ctx->llctx);
  SHOW("Created IR function")
  if (isMainFn) {
    if (ctx->getMod()->hasMainFn()) {
      ctx->Error(ctx->highlightError("main") + " function already exists in this module. Please check the codebase",
                 fileRange);
    } else {
      auto args = fun->getType()->asFunction()->getArgumentTypes();
      if (args.size() == 2) {
        if (args.at(0)->getType()->isUnsignedInteger() &&
            (args.at(0)->getType()->asUnsignedInteger()->getBitwidth() == MainFirstArgBitwidth)) {
          if (args.at(1)->getType()->isPointer() && ((!args.at(1)->getType()->asPointer()->isSubtypeVariable()) &&
                                                     (args.at(1)->getType()->asPointer()->getSubType()->isCString()))) {
            mod->setHasMainFn();
            ctx->hasMain = true;
          } else {
            ctx->Error("The second argument of the " + ctx->highlightError("main") + " function should be " +
                           ctx->highlightError("#[cstring]"),
                       arguments.at(1)->getFileRange());
          }
        } else {
          ctx->Error("The first argument of the " + ctx->highlightError("main") + " function should be " +
                         ctx->highlightError("u32"),
                     arguments.at(0)->getFileRange());
        }
      } else if (args.empty()) {
        mod->setHasMainFn();
        ctx->hasMain = true;
      } else {
        ctx->Error("Main function can either have two arguments, or none at "
                   "all. Please check logic and make necessary changes",
                   fileRange);
      }
      // else {
      //   ctx->Error(
      //       "The " + ctx->highlightError("main") +
      //           " function needs 2 arguments, the first argument should be "
      //           + ctx->highlightError("u32") +
      //           ", and the second argument should be " +
      //           ctx->highlightError("#[cstring]"),
      //       fileRange);
      // }
    }
  }
  return fun;
}

void FunctionPrototype::setVariantName(const String& value) const { variantName = value; }

void FunctionPrototype::unsetVariantName() const { variantName = None; }

void FunctionPrototype::define(IR::Context* ctx) {
  if (!isTemplate()) {
    function = createFunction(ctx);
  }
}

// NOLINTNEXTLINE(misc-unused-parameters)
IR::Value* FunctionPrototype::emit(IR::Context* ctx) {
  if (!isTemplate()) {
    return function;
  } else {
    function = createFunction(ctx);
    return function;
  }
}

Json FunctionPrototype::toJson() const {
  Vec<JsonValue> args;
  for (auto* arg : arguments) {
    auto aJson = Json()
                     ._("name", arg->getName())
                     ._("type", arg->getType() ? arg->getType()->toJson() : Json())
                     ._("fileRange", arg->getFileRange());
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

FunctionDefinition::FunctionDefinition(FunctionPrototype* _prototype, Vec<Sentence*> _sentences,
                                       utils::FileRange _fileRange)
    : Node(std::move(_fileRange)), sentences(std::move(_sentences)), prototype(_prototype) {}

void FunctionDefinition::define(IR::Context* ctx) {
  if (!prototype->isTemplate()) {
    prototype->define(ctx);
  } else {
    auto* tempFn          = new IR::TemplateFunction(prototype->name, prototype->templates, this, ctx->getMod(),
                                                     ctx->getVisibInfo(prototype->visibility));
    prototype->templateFn = tempFn;
  }
}

bool FunctionDefinition::isTemplate() const { return prototype->isTemplate(); }

IR::Value* FunctionDefinition::emit(IR::Context* ctx) {
  if (prototype->isTemplate()) {
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
        localArg = block->newValue("qat'future", fnOrigArgs.at(i)->getType(), fnOrigArgs.at(i)->isVariable());
      } else {
        localArg =
            block->newValue(fnOrigArgs.at(i)->getName(), fnOrigArgs.at(i)->getType(), fnOrigArgs.at(i)->isVariable());
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
    SHOW("About to allocate necessary arguments")
    auto argIRTypes = fnEmit->getType()->asFunction()->getArgumentTypes();
    SHOW("Iteration run for function is: " << fnEmit->getName())
    for (usize i = 0; i < argIRTypes.size(); i++) {
      SHOW("Argument name is " << argIRTypes.at(i)->getName())
      SHOW("Argument type is " << argIRTypes.at(i)->getType()->toString())
      auto* argVal =
          block->newValue(argIRTypes.at(i)->getName(), argIRTypes.at(i)->getType(), argIRTypes.at(i)->isVariable());
      SHOW("Created local value for the argument")
      ctx->builder.CreateStore(fnEmit->getLLVMFunction()->getArg(i), argVal->getAlloca(), false);
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