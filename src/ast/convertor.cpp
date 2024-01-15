#include "convertor.hpp"
#include "../show.hpp"
#include "./expression.hpp"
#include "./sentences/member_initialisation.hpp"
#include "sentence.hpp"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include <algorithm>

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

void ConvertorPrototype::setMemberParent(IR::MemberParent* _memberParent) const { memberParent = _memberParent; }

void ConvertorPrototype::define(IR::Context* ctx) {
  if (!memberParent) {
    ctx->Error("No parent found for this member function", fileRange);
  }
  SHOW("Generating candidate type")
  IR::QatType* candidate = nullptr;
  if (isFrom && argName.has_value() && isMemberArgument) {
    if (memberParent->getParentType()->isCoreType()) {
      auto coreType = memberParent->getParentType()->asCore();
      if (!coreType->hasMember(argName->value)) {
        ctx->Error("No member field named " + ctx->highlightError(argName->value) + " found in core type " +
                       ctx->highlightError(coreType->getFullName()),
                   fileRange);
      }
      coreType->getMember(argName->value)->addMention(argName->range);
      candidate = coreType->getTypeOfMember(argName->value);
    } else if (memberParent->getParentType()->isMix()) {
      auto mixTy  = memberParent->getParentType()->asMix();
      auto mixRes = mixTy->hasSubTypeWithName(argName->value);
      if (!mixRes.first) {
        ctx->Error("No variant named " + ctx->highlightError(argName->value) + " is present in mix type " +
                       ctx->highlightError(mixTy->toString()),
                   argName->range);
      }
      if (!mixRes.second) {
        ctx->Error("The variant named " + ctx->highlightError(argName->value) + " of mix type " +
                       ctx->highlightError(mixTy->toString()) + " does not have a type associated with it",
                   argName->range);
      }
      candidate = mixTy->getSubTypeWithName(argName->value);
    } else {
      ctx->Error(
          "The parent type of this from convertor is not a mix or core type, and hence member argument syntax cannot be used here",
          argName->range);
    }
  } else {
    candidate = candidateType->emit(ctx);
  }
  SHOW("Candidate type generated")
  SHOW("About to create convertor")
  if (isFrom) {
    SHOW("Convertor is FROM")
    memberFn =
        IR::MemberFunction::CreateFromConvertor(memberParent, nameRange, candidate, argName.value(),
                                                definitionRange.value_or(fileRange), ctx->getVisibInfo(visibSpec), ctx);
  } else {
    SHOW("Convertor is TO")
    memberFn = IR::MemberFunction::CreateToConvertor(
        memberParent, nameRange, candidate, definitionRange.value_or(fileRange), ctx->getVisibInfo(visibSpec), ctx);
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
    : Node(std::move(_fileRange)), sentences(std::move(_sentences)), prototype(_prototype) {
  prototype->definitionRange = fileRange;
}

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
  auto* parentRefType = IR::ReferenceType::get(prototype->isFrom, prototype->memberParent->getParentType(), ctx);
  auto* self          = block->newValue("''", parentRefType, false, prototype->memberParent->getTypeRange());
  ctx->builder.CreateStore(fnEmit->getLLVMFunction()->getArg(0), self->getLLVM());
  self->loadImplicitPointer(ctx->builder);
  if (prototype->isFrom) {
    if (prototype->isMemberArgument) {
      llvm::Value* memPtr = nullptr;
      if (parentRefType->getSubType()->isCoreType()) {
        memPtr = ctx->builder.CreateStructGEP(
            parentRefType->getSubType()->getLLVMType(), self->getLLVM(),
            parentRefType->getSubType()->asCore()->getIndexOf(prototype->argName->value).value());
        fnEmit->addInitMember({parentRefType->getSubType()->asCore()->getIndexOf(prototype->argName->value).value(),
                               prototype->argName->range});
      } else {
        memPtr = ctx->builder.CreatePointerCast(
            ctx->builder.CreateStructGEP(parentRefType->getSubType()->getLLVMType(), self->getLLVM(), 1u),
            llvm::PointerType::get(
                parentRefType->getSubType()->asMix()->getSubTypeWithName(prototype->argName->value)->getLLVMType(),
                ctx->dataLayout.value().getProgramAddressSpace()));
        fnEmit->addInitMember({parentRefType->getSubType()->asMix()->getIndexOfName(prototype->argName->value),
                               prototype->argName->range});
      }
      SHOW("Storing member arg in from convertor")
      ctx->builder.CreateStore(fnEmit->getLLVMFunction()->getArg(1), memPtr, false);
      if (parentRefType->getSubType()->isMix()) {
        auto* mixTy = parentRefType->getSubType()->asMix();
        ctx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, mixTy->getTagBitwidth()),
                                                        mixTy->getIndexOfName(prototype->argName->value)),
                                 ctx->builder.CreateStructGEP(mixTy->getLLVMType(), self->getLLVM(), 0u));
      }
      SHOW("Member arg complete")
    } else {
      auto* argTy = fnEmit->getType()->asFunction()->getArgumentTypeAt(1);
      auto* argVal =
          block->newValue(argTy->getName(), argTy->getType(), argTy->isVariable(), prototype->argName->range);
      ctx->builder.CreateStore(fnEmit->getLLVMFunction()->getArg(1), argVal->getLLVM());
    }
  }
  for (auto sent : sentences) {
    if (sent->nodeType() == NodeType::memberInit) {
      ((ast::MemberInit*)sent)->isAllowed = true;
    }
  }
  emitSentences(sentences, ctx);
  if (prototype->isFrom) {
    if (prototype->memberParent->getParentType()->isCoreType()) {
      SHOW("Setting default values for fields")
      auto coreTy = prototype->memberParent->getParentType()->asCore();
      for (usize i = 0; i < coreTy->getMemberCount(); i++) {
        auto mem = coreTy->getMemberAt(i);
        if (fnEmit->isMemberInitted(i)) {
          continue;
        }
        if (mem->defaultValue.has_value()) {
          auto* memVal = mem->defaultValue.value()->emit(ctx);
          if (memVal->getType()->isSame(mem->type)) {
            if (memVal->isImplicitPointer()) {
              if (mem->type->isTriviallyCopyable() || mem->type->isTriviallyMovable()) {
                ctx->builder.CreateStore(ctx->builder.CreateLoad(mem->type->getLLVMType(), memVal->getLLVM()),
                                         ctx->builder.CreateStructGEP(coreTy->getLLVMType(), self->getLLVM(), i));
                if (!mem->type->isTriviallyCopyable()) {
                  if (!memVal->isVariable()) {
                    ctx->Error("This expression does not have variability and hence cannot be trivially moved from",
                               mem->defaultValue.value()->fileRange);
                  }
                  ctx->builder.CreateStore(llvm::Constant::getNullValue(mem->type->getLLVMType()), memVal->getLLVM());
                }
              } else {
                ctx->Error("This expression is of type " + ctx->highlightError(memVal->getType()->toString()) +
                               " which is not trivially copyable or trivially movable. Please use " +
                               ctx->highlightError("'copy") + " or " + ctx->highlightError("'move") + " accordingly",
                           mem->defaultValue.value()->fileRange);
              }
            } else {
              ctx->builder.CreateStore(memVal->getLLVM(),
                                       ctx->builder.CreateStructGEP(coreTy->getLLVMType(), self->getLLVM(), i));
            }
          } else if (memVal->isReference() && memVal->getType()->asReference()->getSubType()->isSame(mem->type)) {
            if (mem->type->isTriviallyCopyable() || mem->type->isTriviallyMovable()) {
              ctx->builder.CreateStore(ctx->builder.CreateLoad(mem->type->getLLVMType(), memVal->getLLVM()),
                                       ctx->builder.CreateStructGEP(coreTy->getLLVMType(), self->getLLVM(), i));
              if (!mem->type->isTriviallyCopyable()) {
                if (!memVal->getType()->asReference()->isSubtypeVariable()) {
                  ctx->Error(
                      "This expression is a reference without variability and hence cannot be trivially moved from",
                      mem->defaultValue.value()->fileRange);
                }
                ctx->builder.CreateStore(llvm::Constant::getNullValue(mem->type->getLLVMType()), memVal->getLLVM());
              }
            } else {
              ctx->Error("This expression is a reference to type " + ctx->highlightError(mem->type->toString()) +
                             " which is not trivially copyable or trivially movable. Please use " +
                             ctx->highlightError("'copy") + " or " + ctx->highlightError("'move") + " accordingly",
                         mem->defaultValue.value()->fileRange);
            }
          } else if (mem->type->isReference() && mem->type->asReference()->getSubType()->isSame(memVal->getType()) &&
                     memVal->isImplicitPointer() &&
                     (mem->type->asReference()->isSubtypeVariable() ? memVal->isVariable() : true)) {
            ctx->builder.CreateStore(memVal->getLLVM(),
                                     ctx->builder.CreateStructGEP(coreTy->getLLVMType(), self->getLLVM(), i));
          } else {
            ctx->Error("The expected type of the member field is " + ctx->highlightError(mem->type->toString()) +
                           " but the value provided is of type " + ctx->highlightError(memVal->getType()->toString()),
                       FileRange{mem->name.range, mem->defaultValue.value()->fileRange});
          }
          fnEmit->addInitMember({i, mem->defaultValue.value()->fileRange});
        } else if (mem->type->hasDefaultValue() || mem->type->isDefaultConstructible()) {
          if (mem->type->hasDefaultValue()) {
            ctx->Warning("Member field " + ctx->highlightWarning(mem->name.value) +
                             " is initialised using the default value of type " +
                             ctx->highlightWarning(mem->type->toString()) +
                             " at the end of this convertor. Try using " + ctx->highlightWarning("default") +
                             " instead to explicitly initialise the member field",
                         fileRange);
            ctx->builder.CreateStore(mem->type->getDefaultValue(ctx)->getLLVM(),
                                     ctx->builder.CreateStructGEP(coreTy->getLLVMType(), self->getLLVM(), i));
          } else {
            ctx->Warning("Member field " + ctx->highlightWarning(mem->name.value) +
                             " is default constructed at the end of this convertor. Try using " +
                             ctx->highlightWarning("default") + " instead to explicitly initialise the member field",
                         fileRange);
            mem->type->defaultConstructValue(
                ctx,
                new IR::Value(ctx->builder.CreateStructGEP(coreTy->getLLVMType(), self->getLLVM(), i),
                              IR::ReferenceType::get(true, mem->type, ctx), false, IR::Nature::temporary),
                fnEmit);
          }
          fnEmit->addInitMember({i, fileRange});
        }
      }
    }
    if (prototype->memberParent->getParentType()->isCoreType()) {
      Vec<Pair<String, FileRange>> missingMembers;
      auto                         cTy = prototype->memberParent->getParentType()->asCore();
      for (auto ind = 0; ind < cTy->getMemberCount(); ind++) {
        auto memCheck = fnEmit->isMemberInitted(ind);
        if (!memCheck.has_value()) {
          missingMembers.push_back({cTy->getMemberAt(ind)->name.value, fileRange});
        }
      }
      if (!missingMembers.empty()) {
        Vec<IR::QatError> errors;
        for (usize i = 0; i < missingMembers.size(); i++) {
          errors.push_back(IR::QatError("Member field " + ctx->highlightError(missingMembers[i].first) +
                                            " of parent type " + ctx->highlightError(cTy->getFullName()) +
                                            " is not initialised in this convertor",
                                        missingMembers[i].second));
        }
        ctx->Errors(errors);
      }
    } else if (prototype->memberParent->getParentType()->isMix()) {
      bool isMixInitialised = false;
      for (usize i = 0; i < prototype->memberParent->getParentType()->asMix()->getSubTypeCount(); i++) {
        auto memRes = fnEmit->isMemberInitted(i);
        if (memRes.has_value()) {
          isMixInitialised = true;
          break;
        }
      }
      if (!isMixInitialised) {
        ctx->Error("Mix type is not initialised in this convertor", fileRange);
      }
    }
  }
  IR::functionReturnHandler(ctx, fnEmit, sentences.empty() ? fileRange : sentences.back()->fileRange);
  SHOW("Sentences emitted")
  (void)ctx->setActiveFunction(oldFn);
  return nullptr;
}

void ConvertorDefinition::setMemberParent(IR::MemberParent* _memberParent) const {
  prototype->setMemberParent(_memberParent);
}

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