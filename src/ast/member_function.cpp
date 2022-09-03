#include "./member_function.hpp"
#include "../show.hpp"
#include "llvm/IR/GlobalValue.h"
#include <vector>

namespace qat::ast {

MemberPrototype::MemberPrototype(bool _isStatic, bool _isVariationFn,
                                 String _name, Vec<Argument *> _arguments,
                                 bool _isVariadic, QatType *_returnType,
                                 bool _is_async, utils::VisibilityKind kind,
                                 const utils::FileRange &_fileRange)
    : Node(_fileRange), isVariationFn(_isVariationFn), name(std::move(_name)),
      isAsync(_is_async), arguments(std::move(_arguments)),
      isVariadic(_isVariadic), returnType(_returnType), kind(kind),
      isStatic(_isStatic) {}

MemberPrototype *MemberPrototype::Normal(
    bool _isVariationFn, const String &_name, const Vec<Argument *> &_arguments,
    bool _isVariadic, QatType *_returnType, bool _is_async,
    utils::VisibilityKind kind, const utils::FileRange &_fileRange) {
  return new MemberPrototype(false, _isVariationFn, _name, _arguments,
                             _isVariadic, _returnType, _is_async, kind,
                             _fileRange);
}

MemberPrototype *MemberPrototype::Static(const String          &_name,
                                         const Vec<Argument *> &_arguments,
                                         bool _isVariadic, QatType *_returnType,
                                         bool                    _is_async,
                                         utils::VisibilityKind   kind,
                                         const utils::FileRange &_fileRange) {
  return new MemberPrototype(true, false, _name, _arguments, _isVariadic,
                             _returnType, _is_async, kind, _fileRange);
}

void MemberPrototype::define(IR::Context *ctx) const {
  if (!coreType) {
    ctx->Error("No core type found for this member function", fileRange);
  }
  Vec<IR::QatType *> generatedTypes;
  // TODO - Check existing member functions
  SHOW("Generating types")
  for (auto *arg : arguments) {
    if (arg->isTypeMember()) {
      if (!isStatic) {
        if (coreType->hasMember(arg->getName())) {
          if (isVariationFn) {
            generatedTypes.push_back(coreType->getTypeOfMember(arg->getName()));
          } else {
            ctx->Error("This member function is not marked as a variation. It "
                       "cannot use the member argument syntax",
                       fileRange);
          }
        } else {
          ctx->Error("No non-static member named " + arg->getName() +
                         " in the core type " + coreType->getFullName(),
                     arg->getFileRange());
        }
      } else {
        ctx->Error("Function " + name +
                       " is a static member function of the core type: " +
                       coreType->getFullName() +
                       ". So it "
                       "cannot use the member argument syntax",
                   arg->getFileRange());
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
  SHOW("About to create function")
  if (isStatic) {
    memberFn = IR::MemberFunction::CreateStatic(
        coreType, name, returnType->emit(ctx), returnType->isVariable(),
        isAsync, args, isVariadic, fileRange, ctx->getVisibInfo(kind),
        ctx->llctx);
  } else {
    memberFn = IR::MemberFunction::Create(
        coreType, isVariationFn, name, returnType->emit(ctx),
        returnType->isVariable(), isAsync, args, isVariadic, fileRange,
        ctx->getVisibInfo(kind), ctx->llctx);
  }
}

IR::Value *MemberPrototype::emit(IR::Context *ctx) { return memberFn; }

void MemberPrototype::setCoreType(IR::CoreType *_coreType) const {
  coreType = _coreType;
}

nuo::Json MemberPrototype::toJson() const {
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
      ._("nodeType", "memberPrototype")
      ._("isVariation", isVariationFn)
      ._("isStatic", isStatic)
      ._("name", name)
      ._("isAsync", isAsync)
      ._("returnType", returnType->toJson())
      ._("arguments", args)
      ._("isVariadic", isVariadic);
}

MemberDefinition::MemberDefinition(MemberPrototype *_prototype,
                                   Vec<Sentence *>  _sentences,
                                   utils::FileRange _fileRange)
    : Node(std::move(_fileRange)), sentences(std::move(_sentences)),
      prototype(_prototype) {}

void MemberDefinition::define(IR::Context *ctx) const {
  prototype->define(ctx);
}

IR::Value *MemberDefinition::emit(IR::Context *ctx) {
  auto *fnEmit = (IR::MemberFunction *)prototype->emit(ctx);
  ctx->fn      = fnEmit;
  SHOW("Set active member function: " << fnEmit->getFullName())
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
          corePtrTy->getSubType()->getLLVMType(), self->getLLVM(),
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
  if (fnEmit->getType()->asFunction()->getReturnType()->isVoid() &&
      (block->getName() == fnEmit->getBlock()->getName())) {
    if (block->getBB()->getInstList().empty()) {
      ctx->builder.CreateRetVoid();
    } else {
      auto *lastInst = ((llvm::Instruction *)&block->getBB()->back());
      if (!llvm::isa<llvm::ReturnInst>(lastInst)) {
        ctx->builder.CreateRetVoid();
      }
    }
  }
  SHOW("Sentences emitted")
  return nullptr;
}

void MemberDefinition::setCoreType(IR::CoreType *coreType) const {
  prototype->setCoreType(coreType);
}

nuo::Json MemberDefinition::toJson() const {
  Vec<nuo::JsonValue> sntcs;
  for (auto *sentence : sentences) {
    sntcs.push_back(sentence->toJson());
  }
  return nuo::Json()
      ._("nodeType", "memberDefinition")
      ._("prototype", prototype->toJson())
      ._("body", sntcs)
      ._("fileRange", fileRange);
}

} // namespace qat::ast