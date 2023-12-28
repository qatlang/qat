#include "./member_initialisation.hpp"

namespace qat::ast {

MemberInit::MemberInit(Identifier _memName, Expression* _value, bool _isInitOfMixVariantWithoutValue,
                       FileRange _fileRange)
    : Sentence(_fileRange), memName(_memName), value(_value) {}

IR::Value* MemberInit::emit(IR::Context* ctx) {
  if (ctx->getActiveFunction()->isMemberFunction()) {
    auto* memFn = (IR::MemberFunction*)ctx->getActiveFunction();
    switch (memFn->getMemberFnType()) {
      case IR::MemberFnType::copyConstructor:
      case IR::MemberFnType::moveConstructor:
      case IR::MemberFnType::defaultConstructor:
      case IR::MemberFnType::fromConvertor:
      case IR::MemberFnType::constructor: {
        if (!isAllowed) {
          ctx->Error(
              "Member initialisation is not supported in this scope. It can only be present in the top level scope of the " +
                  String(memFn->getMemberFnType() == IR::MemberFnType::fromConvertor ? "convertor" : "constructor"),
              fileRange);
        }
        auto*        parentTy = memFn->getParentType();
        IR::QatType* memTy    = nullptr;
        Maybe<usize> memIndex;
        // FIXME - Support tuple types with named member fields?
        auto selfRef = memFn->getFirstBlock()->getValue("''");
        if (parentTy->isCoreType()) {
          if (parentTy->asCore()->hasMember(memName.value)) {
            memTy            = parentTy->asCore()->getTypeOfMember(memName.value);
            memIndex         = parentTy->asCore()->getIndexOf(memName.value);
            auto memCheckRes = memFn->isMemberInitted(memIndex.value());
            if (memCheckRes.has_value()) {
              ctx->Error("Member field " + ctx->highlightError(memName.value) + " is already initialised at " +
                             ctx->highlightError(memCheckRes.value().startToString()),
                         fileRange);
            } else {
              memFn->addInitMember({memIndex.value(), fileRange});
            }
          } else {
            ctx->Error("Parent type " + ctx->highlightError(parentTy->toString()) +
                           " does not have a member field named " + ctx->highlightError(memName.value),
                       fileRange);
          }
        } else if (parentTy->isMix()) {
          auto mixRes = parentTy->asMix()->hasSubTypeWithName(memName.value);
          if (!mixRes.first) {
            ctx->Error("No variant named " + ctx->highlightError(memName.value) + " found in parent mix type " +
                           ctx->highlightError(parentTy->toString()),
                       memName.range);
          }
          for (auto ind = 0; ind < parentTy->asMix()->getSubTypeCount(); ind++) {
            auto memCheckRes = memFn->isMemberInitted(ind);
            if (memCheckRes.has_value()) {
              ctx->Error("The mix type instance has already been initialised at " +
                             ctx->highlightError(memCheckRes.value().startToString()) + ". Cannot initialise again",
                         fileRange);
            }
          }
          memFn->addInitMember({parentTy->asMix()->getIndexOfName(memName.value), fileRange});
          if (!mixRes.second) {
            if (isInitOfMixVariantWithoutValue) {
              ctx->builder.CreateStore(
                  llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, parentTy->asMix()->getTagBitwidth()),
                                         parentTy->asMix()->getIndexOfName(memName.value)),
                  ctx->builder.CreateStructGEP(parentTy->getLLVMType(), selfRef->getLLVM(), 0u));
              return nullptr;
            } else {
              ctx->Error(
                  "The variant " + ctx->highlightError(memName.value) + " of mix type " +
                      ctx->highlightError(parentTy->toString()) +
                      " does not have a type associated with it, and hence cannot be initialised with a value. Please use " +
                      ctx->highlightError("'' is " + memName.value + ".") + " instead",
                  memName.range);
            }
          } else {
            memTy = parentTy->asMix()->getSubTypeWithName(memName.value);
            if (isInitOfMixVariantWithoutValue) {
              auto selfVal = memFn->getFirstBlock()->getValue("''");
              if (memTy->hasDefaultValue()) {
                ctx->builder.CreateStore(
                    memTy->getDefaultValue(ctx)->getLLVM(),
                    ctx->builder.CreatePointerCast(
                        ctx->builder.CreateStructGEP(parentTy->getLLVMType(), selfVal->getLLVM(), 1u),
                        llvm::PointerType::get(memTy->getLLVMType(),
                                               ctx->dataLayout.value().getProgramAddressSpace())));
              } else if (memTy->isDefaultConstructible()) {
                memTy->defaultConstructValue(
                    ctx,
                    new IR::Value(ctx->builder.CreatePointerCast(
                                      ctx->builder.CreateStructGEP(parentTy->getLLVMType(), selfVal->getLLVM(), 1u),
                                      llvm::PointerType::get(memTy->getLLVMType(),
                                                             ctx->dataLayout.value().getProgramAddressSpace())),
                                  IR::ReferenceType::get(true, memTy, ctx), false, IR::Nature::temporary),
                    memFn);
              } else {
                ctx->Error(
                    "The subtype of the variant " + ctx->highlightError(memName.value) + " of mix type " +
                        ctx->highlightError(parentTy->toString()) + " is " + ctx->highlightError(memTy->toString()) +
                        " which does not have a default value and is also not default constructible. Initialisation of the variant"
                        " without value is not supported for this variant",
                    fileRange);
              }
              ctx->builder.CreateStore(
                  llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, parentTy->asMix()->getTagBitwidth()),
                                         parentTy->asMix()->getIndexOfName(memName.value)),
                  ctx->builder.CreateStructGEP(parentTy->getLLVMType(), selfVal->getLLVM(), 0u));
            }
          }
        } else {
          ctx->Error("Cannot use member initialisation for type " + ctx->highlightError(parentTy->toString()) +
                         ". It is only allowed for core & mix types",
                     fileRange);
        }
        if (value->hasTypeInferrance()) {
          value->asTypeInferrable()->setInferenceType(memTy);
        }
        IR::Value* memRef = nullptr;
        if (parentTy->isCoreType()) {
          memRef =
              new IR::Value(ctx->builder.CreateStructGEP(parentTy->getLLVMType(), selfRef->getLLVM(), memIndex.value()),
                            IR::ReferenceType::get(true, memTy, ctx), false, IR::Nature::temporary);
        } else {
          memRef = new IR::Value(
              ctx->builder.CreatePointerCast(
                  ctx->builder.CreateStructGEP(parentTy->getLLVMType(), selfRef->getLLVM(), 1u),
                  llvm::PointerType::get(memTy->getLLVMType(), ctx->dataLayout.value().getProgramAddressSpace())),
              IR::ReferenceType::get(true, memTy, ctx), false, IR::Nature::temporary);
        }
        if (value->isInPlaceCreatable()) {
          value->asInPlaceCreatable()->setCreateIn(memRef);
          (void)value->emit(ctx);
        } else {
          auto* irVal = value->emit(ctx);
          if (memTy->isSame(irVal->getType())) {
            if (!memTy->isReference() && irVal->isImplicitPointer()) {
              if (memTy->isTriviallyCopyable() || memTy->isTriviallyMovable()) {
                ctx->builder.CreateStore(ctx->builder.CreateLoad(memTy->getLLVMType(), irVal->getLLVM()),
                                         memRef->getLLVM());
                if (!memTy->isTriviallyCopyable()) {
                  if (!irVal->isVariable()) {
                    ctx->Error("This expression does not have variability and hence cannot be trivially moved from",
                               value->fileRange);
                  }
                  ctx->builder.CreateStore(llvm::Constant::getNullValue(memTy->getLLVMType()), irVal->getLLVM());
                }
              } else {
                ctx->Error("Member field type " + ctx->highlightError(memTy->toString()) +
                               " cannot be trivially copied or trivially moved. Please use " +
                               ctx->highlightError("'copy") + " or " + ctx->highlightError("'move") + " accordingly",
                           value->fileRange);
              }
            } else {
              ctx->builder.CreateStore(irVal->getLLVM(), memRef->getLLVM());
            }
          } else if (irVal->isReference() && irVal->getType()->asReference()->getSubType()->isSame(memTy)) {
            if (memTy->isTriviallyCopyable() || memTy->isTriviallyMovable()) {
              ctx->builder.CreateStore(ctx->builder.CreateLoad(memTy->getLLVMType(), irVal->getLLVM()),
                                       memRef->getLLVM());
              if (!memTy->isTriviallyCopyable()) {
                if (!irVal->getType()->asReference()->isSubtypeVariable()) {
                  ctx->Error(
                      "This expression is a reference without variability and hence cannot be trivially moved from",
                      value->fileRange);
                }
                ctx->builder.CreateStore(llvm::Constant::getNullValue(memTy->getLLVMType()), irVal->getLLVM());
              }
            } else {
              ctx->Error("Member field type " + ctx->highlightError(memTy->toString()) +
                             " cannot be trivially copied or trivially  moved. Please use " +
                             ctx->highlightError("'copy") + " or " + ctx->highlightError("'move") + " accordingly",
                         value->fileRange);
            }
          } else {
            ctx->Error("Member field type is " + ctx->highlightError(memTy->toString()) +
                           " but the expression is of type " + ctx->highlightError(irVal->getType()->toString()),
                       fileRange);
          }
        }
        if (parentTy->isMix()) {
          ctx->builder.CreateStore(
              llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, parentTy->asMix()->getTagBitwidth()),
                                     parentTy->asMix()->getIndexOfName(memName.value)),
              ctx->builder.CreateStructGEP(parentTy->getLLVMType(), selfRef->getLLVM(), 0u));
        }
        return nullptr;
        break;
      }
      default: {
        ctx->Error("This function is not a constructor for " + ctx->highlightError(memFn->getParentType()->toString()) +
                       " and hence cannot use member initialisation",
                   fileRange);
      }
    }
    return nullptr;
  } else {
    ctx->Error("This function is not a constructor of any type and hence cannot use member initialisation", fileRange);
  }
}

Json MemberInit::toJson() const {
  return Json()
      ._("nodeType", "memberInit")
      ._("memberName", memName)
      ._("value", value->toJson())
      ._("fileRange", fileRange);
}

} // namespace qat::ast