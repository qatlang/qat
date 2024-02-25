#include "./mix_choice_init.hpp"
#include "llvm/Analysis/ConstantFolding.h"

namespace qat::ast {

IR::PrerunValue* PrerunMixOrChoiceInit::emit(IR::Context* ctx) {
  SHOW("Prerun Mix/Choice type initialiser")
  if (!type.has_value() && !isTypeInferred()) {
    ctx->Error("No type is provided for this expression, and no type could be inferred from context", fileRange);
  }
  auto*        typeEmitOrig = type.has_value() ? type.value()->emit(ctx) : nullptr;
  IR::QatType* typeEmit;
  if (type.has_value()) {
    if (typeEmitOrig->getType()->isTyped()) {
      typeEmit = typeEmitOrig->getType()->asTyped()->getSubType();
      if (isTypeInferred() && !typeEmit->isSame(inferredType)) {
        ctx->Error("The type provided is " + ctx->highlightError(typeEmit->toString()) +
                       " but the type inferred from scope is " + ctx->highlightError(inferredType->toString()),
                   type.value()->fileRange);
      }
    } else {
      ctx->Error("Expected a type here, but got a value of type " +
                     ctx->highlightError(typeEmitOrig->getType()->toString()),
                 type.value()->fileRange);
    }
  } else if (isTypeInferred()) {
    typeEmit = inferredType;
  } else {
    ctx->Error("No type provided here, and no type could be inferred from scope", fileRange);
  }
  if (typeEmit->isMix()) {
    auto* mixTy = typeEmit->asMix();
    if (!mixTy->canBePrerun()) {
      ctx->Error("Mix type " + ctx->highlightError(mixTy->toString()) + " cannot be used in a prerun expression",
                 fileRange);
    }
    auto subRes = mixTy->hasSubTypeWithName(subName.value);
    SHOW("Subtype check")
    if (subRes.first) {
      if (subRes.second) {
        if (!expression.has_value()) {
          ctx->Error("Variant " + ctx->highlightError(subName.value) + " of mix type " +
                         ctx->highlightError(mixTy->getFullName()) + " expects a value to be associated with it",
                     fileRange);
        }
      } else {
        if (expression.has_value()) {
          ctx->Error("Variant " + ctx->highlightError(subName.value) + " of mix type " +
                         ctx->highlightError(mixTy->getFullName()) + " cannot have any value associated with it",
                     fileRange);
        }
      }
      llvm::Constant* exp = nullptr;
      if (subRes.second) {
        auto* typ = mixTy->getSubTypeWithName(subName.value);
        if (expression.value()->hasTypeInferrance()) {
          expression.value()->asTypeInferrable()->setInferenceType(typ);
        }
        auto* expEmit = expression.value()->emit(ctx);
        if (typ->isSame(expEmit->getType())) {
          exp           = expEmit->getLLVMConstant();
          auto typeBits = (u64)ctx->dataLayout.value().getTypeStoreSizeInBits(typ->getLLVMType());
          exp           = llvm::ConstantFoldConstant(
              llvm::ConstantExpr::getIntegerCast(
                  llvm::ConstantExpr::getBitCast(exp, llvm::Type::getIntNTy(ctx->llctx, typeBits)),
                  llvm::Type::getIntNTy(ctx->llctx, typeBits), false),
              ctx->dataLayout.value());
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
      return new IR::PrerunValue(
          llvm::ConstantStruct::get(llvm::cast<llvm::StructType>(mixTy->getLLVMType()), {index, exp}), mixTy);
    } else {
      ctx->Error("No field named " + ctx->highlightError(subName.value) + " is present inside mix type " +
                     ctx->highlightError(mixTy->getFullName()),
                 fileRange);
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
      return new IR::PrerunValue(chTy->getValueFor(subName.value), chTy);
    } else {
      ctx->Error("Choice type " + ctx->highlightError(chTy->toString()) + " does not have a variant named " +
                     ctx->highlightError(subName.value),
                 subName.range);
    }
  } else {
    ctx->Error(ctx->highlightError(typeEmit->toString()) +
                   " is not a mix type or a choice type and hence cannot be used here",
               fileRange);
  }
  return nullptr;
}

String PrerunMixOrChoiceInit::toString() const {
  return (type.has_value() ? type.value()->toString() : "") + "::" + subName.value +
         (expression.has_value() ? ("(" + expression.value()->toString() + ")") : "");
}

Json PrerunMixOrChoiceInit::toJson() const {
  return Json()
      ._("nodeType", "prerunMixOrChoiceInit")
      ._("hasType", type.has_value())
      ._("type", type.has_value() ? type.value()->toJson() : JsonValue())
      ._("subName", subName)
      ._("hasExpression", expression.has_value())
      ._("expression", expression.has_value() ? expression.value()->toJson() : JsonValue())
      ._("fileRange", fileRange);
}

} // namespace qat::ast