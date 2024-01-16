#include "./mix_choice_initialiser.hpp"

namespace qat::ast {

IR::Value* MixOrChoiceInitialiser::emit(IR::Context* ctx) {
  // FIXME - Support heaped value
  SHOW("Mix/Choice type initialiser")
  auto reqInfo = ctx->getAccessInfo();
  if (!type.has_value() && !isTypeInferred()) {
    ctx->Error("No type is provided for this expression, and no type could be inferred from context", fileRange);
  }
  auto* typeEmit = type.has_value() ? type.value()->emit(ctx) : inferredType;
  if (typeEmit->isMix()) {
    auto* mixTy = typeEmit->asMix();
    if (mixTy->isAccessible(ctx->getAccessInfo())) {
      auto subRes = mixTy->hasSubTypeWithName(subName.value);
      SHOW("Subtype check")
      if (subRes.first) {
        if (subRes.second) {
          if (!expression.has_value()) {
            ctx->Error("Field " + ctx->highlightError(subName.value) + " of mix type " +
                           ctx->highlightError(mixTy->getFullName()) + " expects a value to be associated with it",
                       fileRange);
          }
        } else {
          if (expression.has_value()) {
            ctx->Error("Field " + ctx->highlightError(subName.value) + " of mix type " +
                           ctx->highlightError(mixTy->getFullName()) + " cannot have any value associated with it",
                       fileRange);
          }
        }
        llvm::Value* exp = nullptr;
        if (subRes.second) {
          auto* typ = mixTy->getSubTypeWithName(subName.value);
          if (expression.value()->hasTypeInferrance()) {
            expression.value()->asTypeInferrable()->setInferenceType(typ);
          }
          auto* expEmit = expression.value()->emit(ctx);
          if (typ->isSame(expEmit->getType())) {
            expEmit->loadImplicitPointer(ctx->builder);
            exp = expEmit->getLLVM();
          } else if (expEmit->isReference() && expEmit->getType()->asReference()->getSubType()->isSame(typ)) {
            exp = ctx->builder.CreateLoad(expEmit->getType()->asReference()->getSubType()->getLLVMType(),
                                          expEmit->getLLVM());
          } else if (typ->isReference() && typ->asReference()->getSubType()->isSame(expEmit->getType())) {
            if (expEmit->isImplicitPointer()) {
              exp = expEmit->getLLVM();
            } else {
              ctx->Error("The expected type is " + ctx->highlightError(typ->toString()) +
                             ", but the expression is of type " + ctx->highlightError(expEmit->getType()->toString()),
                         expression.value()->fileRange);
            }
          } else {
            ctx->Error("The expected type is " + ctx->highlightError(typ->toString()) +
                           ", but the expression is of type " + ctx->highlightError(expEmit->getType()->toString()),
                       expression.value()->fileRange);
          }
        } else {
          exp = llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, mixTy->getDataBitwidth()), 0u);
        }
        auto* index = llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, mixTy->getTagBitwidth()),
                                             mixTy->getIndexOfName(subName.value));
        if (isLocalDecl()) {
          SHOW("Is local decl")
          if (localValue->getType()->isSame(mixTy)) {
            createIn = localValue->toNewIRValue();
          } else {
            ctx->Error("The type of the local declaration is not compatible here", fileRange);
          }
        } else if (canCreateIn()) {
          SHOW("Is createIn")
          if (createIn->isReference() || createIn->isImplicitPointer()) {
            auto expTy =
                createIn->isImplicitPointer() ? createIn->getType() : createIn->getType()->asReference()->getSubType();
            if (!expTy->isSame(mixTy)) {
              ctx->Error(
                  "Trying to optimise the mix type initialisation by creating in-place, but the expression type is " +
                      ctx->highlightError(mixTy->toString()) +
                      " which does not match with the underlying type for in-place creation which is " +
                      ctx->highlightError(expTy->toString()),
                  fileRange);
            }
          } else {
            ctx->Error("Trying to optimise the copy by creating in-place, but the containing type is not a reference",
                       fileRange);
          }
        } else {
          createIn = ctx->getActiveFunction()->getBlock()->newValue(
              irName.has_value() ? irName->value : utils::unique_id(), mixTy, isVar,
              irName.has_value() ? irName->range : fileRange);
        }
        SHOW("Creating mix store")
        ctx->builder.CreateStore(index, ctx->builder.CreateStructGEP(mixTy->getLLVMType(), createIn->getLLVM(), 0));
        if (subRes.second) {
          ctx->builder.CreateStore(exp, ctx->builder.CreatePointerCast(
                                            ctx->builder.CreateStructGEP(mixTy->getLLVMType(), createIn->getLLVM(), 1),
                                            mixTy->getSubTypeWithName(subName.value)->getLLVMType()->getPointerTo()));
        }
        return createIn;
      } else {
        ctx->Error("No field named " + ctx->highlightError(subName.value) + " is present inside mix type " +
                       ctx->highlightError(mixTy->getFullName()),
                   fileRange);
      }
    } else {
      ctx->Error("Mix type " + ctx->highlightError(mixTy->getFullName()) + " is not accessible here", fileRange);
    }
  } else if (typeEmit->isChoice()) {
    if (expression.has_value()) {
      ctx->Error("An expression is provided here, but the recognised type is a choice type: " +
                     ctx->highlightError(typeEmit->toString()) +
                     ". Please remove the expression or check the logic here",
                 expression.value()->fileRange);
    }
    auto* chTy = typeEmit->asChoice();
    if (chTy->hasField(subName.value)) {
      if (isLocalDecl()) {
        ctx->builder.CreateStore(chTy->getValueFor(subName.value), localValue->getLLVM());
        return nullptr;
      } else if (canCreateIn()) {
        if (createIn->isReference() || createIn->isImplicitPointer()) {
          auto expTy =
              createIn->isImplicitPointer() ? createIn->getType() : createIn->getType()->asReference()->getSubType();
          if (!expTy->isSame(chTy)) {
            ctx->Error(
                "Trying to optimise the choice type initialisation by creating in-place, but the expression type is " +
                    ctx->highlightError(chTy->toString()) +
                    " which does not match with the underlying type for in-place creation which is " +
                    ctx->highlightError(expTy->toString()),
                fileRange);
          }
        } else {
          ctx->Error(
              "Trying to optimise the choice type initialisation by creating in-place, but the containing type is not a reference",
              fileRange);
        }
        ctx->builder.CreateStore(chTy->getValueFor(subName.value), createIn->getLLVM());
      } else if (irName.has_value()) {
        auto locVal = ctx->getActiveFunction()->getBlock()->newValue(irName->value, chTy, isVar, irName->range);
        ctx->builder.CreateStore(chTy->getValueFor(subName.value), locVal->getLLVM());
        return nullptr;
      } else {
        return new IR::PrerunValue(chTy->getValueFor(subName.value), chTy);
      }
    } else {
      ctx->Error("Choice type " + ctx->highlightError(chTy->toString()) + " does not have a variant named " +
                     ctx->highlightError(subName.value),
                 subName.range);
    }
  } else {
    ctx->Error(ctx->highlightError(typeEmit->toString()) + " is not a mix type or a choice type", fileRange);
  }
  return nullptr;
}

Json MixOrChoiceInitialiser::toJson() const {
  return Json()
      ._("nodeType", "mixTypeInitialiser")
      ._("hasType", type.has_value())
      ._("type", type.has_value() ? type.value()->toJson() : JsonValue())
      ._("subName", subName)
      ._("hasExpression", expression.has_value())
      ._("expression", expression.has_value() ? expression.value()->toJson() : JsonValue())
      ._("fileRange", fileRange);
}

} // namespace qat::ast