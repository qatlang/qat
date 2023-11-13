#include "convertor.hpp"
#include "../show.hpp"
#include "sentence.hpp"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"

namespace qat::ast {

ConvertorPrototype::ConvertorPrototype(bool _isFrom, FileRange _nameRange, Maybe<Identifier> _argName,
                                       QatType* _candidateType, bool _isMemberArg, Maybe<VisibilitySpec> _visibSpec,
                                       const FileRange& _fileRange)
    : Node(_fileRange), argName(std::move(_argName)), candidateType(_candidateType), isMemberArgument(_isMemberArg),
      visibSpec(_visibSpec), isFrom(_isFrom), nameRange(std::move(_nameRange)) {}

ConvertorPrototype* ConvertorPrototype::From(FileRange _nameRange, Maybe<Identifier> _argName, QatType* _candidateType,
                                             bool _isMemberArg, Maybe<VisibilitySpec> _visibSpec,
                                             const FileRange& _fileRange) {
  return new ConvertorPrototype(true, std::move(_nameRange), _argName, _candidateType, _isMemberArg, _visibSpec,
                                _fileRange);
}

ConvertorPrototype* ConvertorPrototype::To(FileRange _nameRange, QatType* _candidateType,
                                           Maybe<VisibilitySpec> _visibSpec, const FileRange& _fileRange) {
  return new ConvertorPrototype(false, std::move(_nameRange), None, _candidateType, false, _visibSpec, _fileRange);
}

void ConvertorPrototype::setCoreType(IR::CoreType* _coreType) const { coreType = _coreType; }

void ConvertorPrototype::define(IR::Context* ctx) {
  if (!coreType) {
    ctx->Error("No core type found for this member function", fileRange);
  }
  SHOW("Generating candidate type")
  if (isMemberArgument) {
    if (!coreType->hasMember(argName->value)) {
      ctx->Error("No member field named " + ctx->highlightError(argName->value) + " found in core type " +
                     ctx->highlightError(coreType->getFullName()),
                 fileRange);
    }
    coreType->getMember(argName->value)->addMention(argName->range);
  }
  auto* candidate = isMemberArgument ? coreType->getTypeOfMember(argName->value) : candidateType->emit(ctx);
  SHOW("Candidate type generated")
  SHOW("About to create convertor")
  if (isFrom) {
    SHOW("Convertor is FROM")
    memberFn = IR::MemberFunction::CreateFromConvertor(coreType, nameRange, candidate, argName.value(), fileRange,
                                                       ctx->getVisibInfo(visibSpec), ctx);
  } else {
    SHOW("Convertor is TO")
    memberFn = IR::MemberFunction::CreateToConvertor(coreType, nameRange, candidate, fileRange,
                                                     ctx->getVisibInfo(visibSpec), ctx);
  }
}

IR::Value* ConvertorPrototype::emit(IR::Context* ctx) { return memberFn; }

Json ConvertorPrototype::toJson() const {
  return Json()
      ._("nodeType", "convertorPrototype")
      ._("isFrom", isFrom)
      ._("hasArgument", argName.has_value())
      ._("argumentName", argName.has_value() ? argName.value().operator JsonValue() : Json())
      ._("candidateType", candidateType->toJson())
      ._("hasVisibility", visibSpec.has_value())
      ._("visibility", visibSpec.has_value() ? visibSpec->toJson() : JsonValue());
}

ConvertorDefinition::ConvertorDefinition(ConvertorPrototype* _prototype, Vec<Sentence*> _sentences,
                                         FileRange _fileRange)
    : Node(std::move(_fileRange)), sentences(std::move(_sentences)), prototype(_prototype) {}

void ConvertorDefinition::define(IR::Context* ctx) { prototype->define(ctx); }

IR::Value* ConvertorDefinition::emit(IR::Context* ctx) {
  auto* fnEmit = (IR::MemberFunction*)prototype->emit(ctx);
  auto* oldFn  = ctx->setActiveFunction(fnEmit);
  SHOW("Set active convertor function: " << fnEmit->getFullName())
  auto* block = new IR::Block((IR::Function*)fnEmit, nullptr);
  SHOW("Created entry block")
  block->setActive(ctx->builder);
  SHOW("Set new block as the active block")
  SHOW("About to allocate necessary arguments")
  auto* coreRefType = IR::ReferenceType::get(prototype->isFrom, prototype->coreType, ctx);
  auto* self        = block->newValue("''", coreRefType, false, coreRefType->getSubType()->asCore()->getName().range);
  ctx->builder.CreateStore(fnEmit->getLLVMFunction()->getArg(0), self->getLLVM());
  self->loadImplicitPointer(ctx->builder);
  if (prototype->isFrom) {
    if (prototype->isMemberArgument) {
      auto* memPtr = ctx->builder.CreateStructGEP(
          coreRefType->getSubType()->getLLVMType(), self->getLLVM(),
          coreRefType->getSubType()->asCore()->getIndexOf(prototype->argName->value).value());
      ctx->builder.CreateStore(fnEmit->getLLVMFunction()->getArg(1), memPtr, false);
    } else {
      auto* argTy = fnEmit->getType()->asFunction()->getArgumentTypeAt(1);
      auto* argVal =
          block->newValue(argTy->getName(), argTy->getType(), argTy->isVariable(), prototype->argName->range);
      ctx->builder.CreateStore(fnEmit->getLLVMFunction()->getArg(1), argVal->getLLVM());
    }
  }
  emitSentences(sentences, ctx);
  IR::functionReturnHandler(ctx, fnEmit, sentences.empty() ? fileRange : sentences.back()->fileRange);
  SHOW("Sentences emitted")
  (void)ctx->setActiveFunction(oldFn);
  return nullptr;
}

void ConvertorDefinition::setCoreType(IR::CoreType* coreType) const { prototype->setCoreType(coreType); }

Json ConvertorDefinition::toJson() const {
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