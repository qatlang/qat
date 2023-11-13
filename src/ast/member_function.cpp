#include "./member_function.hpp"
#include "../IR/logic.hpp"
#include "../IR/types/future.hpp"
#include "../show.hpp"
#include "llvm/IR/GlobalValue.h"
#include <vector>

namespace qat::ast {

MemberPrototype::MemberPrototype(bool _isStatic, bool _isVariationFn, Identifier _name, Vec<Argument*> _arguments,
                                 bool _isVariadic, QatType* _returnType, bool _is_async,
                                 Maybe<VisibilitySpec> _visibSpec, FileRange _fileRange)
    : Node(std::move(_fileRange)), isVariationFn(_isVariationFn), name(std::move(_name)), isAsync(_is_async),
      arguments(std::move(_arguments)), isVariadic(_isVariadic), returnType(_returnType), visibSpec(_visibSpec),
      isStatic(_isStatic) {}

MemberPrototype::~MemberPrototype() {
  for (auto* arg : arguments) {
    delete arg;
  }
}

MemberPrototype* MemberPrototype::Normal(bool _isVariationFn, const Identifier& _name, const Vec<Argument*>& _arguments,
                                         bool _isVariadic, QatType* _returnType, bool _is_async,
                                         Maybe<VisibilitySpec> visibSpec, const FileRange& _fileRange) {
  return new MemberPrototype(false, _isVariationFn, _name, _arguments, _isVariadic, _returnType, _is_async, visibSpec,
                             _fileRange);
}

MemberPrototype* MemberPrototype::Static(const Identifier& _name, const Vec<Argument*>& _arguments, bool _isVariadic,
                                         QatType* _returnType, bool _is_async, Maybe<VisibilitySpec> visibSpec,
                                         const FileRange& _fileRange) {
  return new MemberPrototype(true, false, _name, _arguments, _isVariadic, _returnType, _is_async, visibSpec,
                             _fileRange);
}

void MemberPrototype::define(IR::Context* ctx) {
  if (!coreType) {
    ctx->Error("No core type found for this member function", fileRange);
  }
  if (coreType->hasMemberFunction(name.value)) {
    ctx->Error("Member function named " + ctx->highlightError(name.value) + " already exists in core type " +
                   ctx->highlightError(coreType->getFullName()),
               fileRange);
  }
  auto* retTy = returnType->emit(ctx);
  if (isAsync) {
    if (!retTy->isFuture()) {
      ctx->Error("An async function should always return a future", fileRange);
    }
  }
  Vec<IR::QatType*> generatedTypes;
  // TODO - Check existing member functions
  SHOW("Generating types")
  for (auto* arg : arguments) {
    if (arg->isTypeMember()) {
      if (!isStatic) {
        if (coreType->hasMember(arg->getName().value)) {
          if (isVariationFn) {
            auto* memTy = coreType->getTypeOfMember(arg->getName().value);
            if (memTy->isReference()) {
              if (memTy->asReference()->isSubtypeVariable()) {
                memTy = memTy->asReference()->getSubType();
              } else {
                ctx->Error("Member " + ctx->highlightError(arg->getName().value) + " of core type " +
                               ctx->highlightError(coreType->getFullName()) +
                               " is not a variable reference and hence cannot "
                               "be reassigned",
                           arg->getName().range);
              }
            }
            generatedTypes.push_back(memTy);
          } else {
            ctx->Error("This member function is not marked as a variation. It "
                       "cannot use the member argument syntax",
                       fileRange);
          }
        } else {
          ctx->Error("No non-static member named " + arg->getName().value + " in the core type " +
                         coreType->getFullName(),
                     arg->getName().range);
        }
      } else {
        ctx->Error("Function " + name.value +
                       " is a static member function of the core type: " + coreType->getFullName() +
                       ". So it "
                       "cannot use the member argument syntax",
                   arg->getName().range);
      }
    } else {
      generatedTypes.push_back(arg->getType()->emit(ctx));
    }
  }
  SHOW("Argument types generated")
  Vec<IR::Argument> args;
  SHOW("Setting variability of arguments")
  for (usize i = 0; i < generatedTypes.size(); i++) {
    if (arguments.at(i)->isTypeMember()) {
      SHOW("Argument at " << i << " named " << arguments.at(i)->getName().value << " is a type member")
      args.push_back(IR::Argument::CreateMember(arguments.at(i)->getName(), generatedTypes.at(i), i));
    } else {
      SHOW("Argument at " << i << " named " << arguments.at(i)->getName().value << " is not a type member")
      args.push_back(
          arguments.at(i)->isVariable()
              ? IR::Argument::CreateVariable(arguments.at(i)->getName(), arguments.at(i)->getType()->emit(ctx), i)
              : IR::Argument::Create(arguments.at(i)->getName(), generatedTypes.at(i), i));
    }
  }
  SHOW("Variability setting complete")
  SHOW("About to create function")
  if (isStatic) {
    memberFn = IR::MemberFunction::CreateStatic(coreType, name, retTy, isAsync, args, isVariadic, fileRange,
                                                ctx->getVisibInfo(visibSpec), ctx);
  } else {
    memberFn = IR::MemberFunction::Create(coreType, isVariationFn, name, retTy, isAsync, args, isVariadic, fileRange,
                                          ctx->getVisibInfo(visibSpec), ctx);
  }
}

IR::Value* MemberPrototype::emit(IR::Context* ctx) { return memberFn; }

void MemberPrototype::setCoreType(IR::CoreType* _coreType) const { coreType = _coreType; }

Json MemberPrototype::toJson() const {
  Vec<JsonValue> args;
  for (auto* arg : arguments) {
    auto aJson = Json()
                     ._("name", arg->getName())
                     ._("type", arg->getType() ? arg->getType()->toJson() : Json())
                     ._("isMemberArg", arg->isTypeMember());
    args.push_back(aJson);
  }
  return Json()
      ._("nodeType", "memberPrototype")
      ._("isVariation", isVariationFn)
      ._("isStatic", isStatic)
      ._("name", name)
      ._("isAsync", isAsync)
      ._("returnType", returnType->toJson())
      ._("arguments", args)
      ._("isVariadic", isVariadic);
}

MemberDefinition::MemberDefinition(MemberPrototype* _prototype, Vec<Sentence*> _sentences, FileRange _fileRange)
    : Node(std::move(_fileRange)), sentences(std::move(_sentences)), prototype(_prototype) {}

void MemberDefinition::define(IR::Context* ctx) { prototype->define(ctx); }

IR::Value* MemberDefinition::emit(IR::Context* ctx) {
  auto* fnEmit = (IR::MemberFunction*)prototype->emit(ctx);
  ctx->setActiveFunction(fnEmit);
  SHOW("Set active member function: " << fnEmit->getFullName())
  if (prototype->isAsync) {
    auto* fun        = fnEmit->getAsyncSubFunction();
    auto* entryBlock = llvm::BasicBlock::Create(ctx->llctx, "entry", fnEmit->getLLVMFunction());
    ctx->builder.SetInsertPoint(entryBlock);
    SHOW("Getting void pointer")
    auto* voidPtrTy = llvm::Type::getInt8Ty(ctx->llctx)->getPointerTo();
    ctx->getMod()->linkNative(IR::NativeUnit::pthreadCreate);
    ctx->getMod()->linkNative(IR::NativeUnit::pthreadAttrInit);
    SHOW("Linked pthread_create & pthread_attr_init functions to current module")
    auto* pthreadCreate     = ctx->getMod()->getLLVMModule()->getFunction("pthread_create");
    auto* pthreadAttrInitFn = ctx->getMod()->getLLVMModule()->getFunction("pthread_attr_init");
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
      if (i == 0) {
        localArg = block->newValue("''", fnOrigArgs.at(0)->getType(), false,
                                   fnOrigArgs.at(0)->getType()->asReference()->getSubType()->asCore()->getName().range);
      } else if (i == (fnOrigArgs.size() - 1)) {
        localArg = block->newValue("qat'future", fnOrigArgs.at(i)->getType(), fnOrigArgs.at(i)->isVariable(), {""});
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
      if (i == 0) {
        localArg->loadImplicitPointer(ctx->builder);
      }
      SHOW("Arg val stored")
    }
  } else {
    auto* block = new IR::Block(fnEmit, nullptr);
    SHOW("Created entry block")
    block->setActive(ctx->builder);
    SHOW("Set new block as the active block")
    SHOW("About to allocate necessary arguments")
    auto  argIRTypes = fnEmit->getType()->asFunction()->getArgumentTypes();
    auto* coreRefTy  = argIRTypes.at(0)->getType()->asReference();
    auto* self       = block->newValue("''", coreRefTy, false, coreRefTy->getSubType()->asCore()->getName().range);
    ctx->builder.CreateStore(fnEmit->getLLVMFunction()->getArg(0u), self->getLLVM());
    self->loadImplicitPointer(ctx->builder);
    SHOW("Arguments size is " << argIRTypes.size())
    for (usize i = 1; i < argIRTypes.size(); i++) {
      SHOW("Argument name in member function is " << argIRTypes.at(i)->getName())
      SHOW("Argument type in member function is " << argIRTypes.at(i)->getType()->toString())
      if (argIRTypes.at(i)->isMemberArgument()) {
        auto* memPtr = ctx->builder.CreateStructGEP(
            coreRefTy->getSubType()->getLLVMType(), self->getLLVM(),
            coreRefTy->getSubType()->asCore()->getIndexOf(argIRTypes.at(i)->getName()).value());
        auto* memTy = coreRefTy->getSubType()->asCore()->getTypeOfMember(argIRTypes.at(i)->getName());
        if (memTy->isReference()) {
          memPtr = ctx->builder.CreateLoad(memTy->asReference()->getLLVMType(), memPtr);
        }
        ctx->builder.CreateStore(fnEmit->getLLVMFunction()->getArg(i), memPtr, false);
      } else {
        SHOW("Argument is variable")
        auto* argVal = block->newValue(argIRTypes.at(i)->getName(), argIRTypes.at(i)->getType(),
                                       argIRTypes.at(i)->isVariable(), prototype->arguments.at(i - 1)->getName().range);
        SHOW("Created local value for the argument")
        ctx->builder.CreateStore(fnEmit->getLLVMFunction()->getArg(i), argVal->getAlloca(), false);
      }
    }
  }
  emitSentences(sentences, ctx);
  IR::functionReturnHandler(ctx, fnEmit, sentences.empty() ? fileRange : sentences.back()->fileRange);
  SHOW("Sentences emitted")
  return nullptr;
}

void MemberDefinition::setCoreType(IR::CoreType* coreType) const { prototype->setCoreType(coreType); }

Json MemberDefinition::toJson() const {
  Vec<JsonValue> sntcs;
  for (auto* sentence : sentences) {
    sntcs.push_back(sentence->toJson());
  }
  return Json()
      ._("nodeType", "memberDefinition")
      ._("prototype", prototype->toJson())
      ._("body", sntcs)
      ._("fileRange", fileRange);
}

} // namespace qat::ast