#include "convertor.hpp"
#include "../show.hpp"
#include "sentence.hpp"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"

namespace qat::ast {

ConvertorPrototype::ConvertorPrototype(bool _isFrom, String _argName,
                                       QatType                *_candidateType,
                                       utils::VisibilityKind   _visibility,
                                       const utils::FileRange &_fileRange)
    : Node(_fileRange), argName(std::move(_argName)),
      candidateType(_candidateType), visibility(_visibility), isFrom(_isFrom) {}

ConvertorPrototype *
ConvertorPrototype::From(const String &_argName, QatType *_candidateType,
                         utils::VisibilityKind   _visibility,
                         const utils::FileRange &_fileRange) {
  return new ConvertorPrototype(true, _argName, _candidateType, _visibility,
                                _fileRange);
}

ConvertorPrototype *ConvertorPrototype::To(QatType              *_candidateType,
                                           utils::VisibilityKind _visibility,
                                           const utils::FileRange &_fileRange) {
  return new ConvertorPrototype(false, "", _candidateType, _visibility,
                                _fileRange);
}

void ConvertorPrototype::setCoreType(IR::CoreType *_coreType) const {
  coreType = _coreType;
}

void ConvertorPrototype::define(IR::Context *ctx) {
  if (!coreType) {
    ctx->Error("No core type found for this member function", fileRange);
  }
  SHOW("Generating candidate type")
  auto *candidate = candidateType->emit(ctx);
  SHOW("Candidate type generated")
  SHOW("About to create convertor")
  if (isFrom) {
    SHOW("Convertor is FROM")
    memberFn = IR::MemberFunction::CreateFromConvertor(
        coreType, candidate, argName, fileRange, ctx->getVisibInfo(visibility),
        ctx->llctx);
  } else {
    SHOW("Convertor is TO")
    memberFn = IR::MemberFunction::CreateToConvertor(
        coreType, candidate, fileRange, ctx->getVisibInfo(visibility),
        ctx->llctx);
  }
}

IR::Value *ConvertorPrototype::emit(IR::Context *ctx) { return memberFn; }

nuo::Json ConvertorPrototype::toJson() const {
  return nuo::Json()
      ._("nodeType", "convertorPrototype")
      ._("isFrom", isFrom)
      ._("argumentName", argName)
      ._("candidateType", candidateType->toJson())
      ._("visibility", utils::kindToJsonValue(visibility));
}

ConvertorDefinition::ConvertorDefinition(ConvertorPrototype *_prototype,
                                         Vec<Sentence *>     _sentences,
                                         utils::FileRange    _fileRange)
    : Node(std::move(_fileRange)), sentences(std::move(_sentences)),
      prototype(_prototype) {}

void ConvertorDefinition::define(IR::Context *ctx) { prototype->define(ctx); }

IR::Value *ConvertorDefinition::emit(IR::Context *ctx) {
  auto *fnEmit = (IR::MemberFunction *)prototype->emit(ctx);
  ctx->fn      = fnEmit;
  SHOW("Set active convertor function: " << fnEmit->getFullName())
  auto *block = new IR::Block((IR::Function *)fnEmit, nullptr);
  SHOW("Created entry block")
  block->setActive(ctx->builder);
  SHOW("Set new block as the active block")
  SHOW("About to allocate necessary arguments")
  auto *corePtrType =
      IR::PointerType::get(prototype->isFrom, prototype->coreType, ctx->llctx);
  auto *self = block->newValue("''", corePtrType, false);
  ctx->builder.CreateStore(fnEmit->getLLVMFunction()->getArg(0),
                           self->getLLVM());
  ctx->selfVal =
      ctx->builder.CreateLoad(self->getType()->getLLVMType(), self->getLLVM());
  if (prototype->isFrom) {
    auto *argTy  = fnEmit->getType()->asFunction()->getArgumentTypeAt(1);
    auto *argVal = block->newValue(argTy->getName(), argTy->getType(),
                                   argTy->isVariable());
    ctx->builder.CreateStore(fnEmit->getLLVMFunction()->getArg(1),
                             argVal->getLLVM());
  }
  emitSentences(sentences, ctx);
  ctx->selfVal = nullptr;
  if (prototype->isFrom &&
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

void ConvertorDefinition::setCoreType(IR::CoreType *coreType) const {
  prototype->setCoreType(coreType);
}

nuo::Json ConvertorDefinition::toJson() const {
  Vec<nuo::JsonValue> sntcs;
  for (auto *sentence : sentences) {
    sntcs.push_back(sentence->toJson());
  }
  return nuo::Json()
      ._("nodeType", "functionDefinition")
      ._("prototype", prototype->toJson())
      ._("body", sntcs)
      ._("fileRange", fileRange);
}

} // namespace qat::ast