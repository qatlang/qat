#include "./array_literal.hpp"
#include "../../IR/types/maybe.hpp"
#include "llvm/IR/Constants.h"

namespace qat::ast {

IR::Value* ArrayLiteral::emit(IR::Context* ctx) {
  IR::ArrayType* arrTyOfLocal = nullptr;
  if (isLocalDecl()) {
    if (localValue->getType()->isArray()) {
      arrTyOfLocal = localValue->getType()->asArray();
    } else {
      ctx->Error("The type for the local declaration is " + ctx->highlightError(localValue->getType()->toString()) +
                     ". This expression expects to be of array type",
                 fileRange);
    }
  }
  if (isLocalDecl() && (arrTyOfLocal->getLength() != values.size())) {
    ctx->Error("The expected length of the array is " + ctx->highlightError(std::to_string(arrTyOfLocal->getLength())) +
                   ", but this array literal has " + ctx->highlightError(std::to_string(values.size())) + " elements",
               fileRange);
  } else if (isTypeInferred() && inferredType->isArray() && inferredType->asArray()->getLength() != values.size()) {
    auto infLen = inferredType->asArray()->getLength();
    ctx->Error("The length of the array type inferred is " + ctx->highlightError(std::to_string(infLen)) + ", but " +
                   ((values.size() < infLen) ? "only " : "") + ctx->highlightError(std::to_string(values.size())) +
                   " value" + ((values.size() > 1u) ? "s were" : "was") + " provided.",
               fileRange);
  } else if (isTypeInferred() && !inferredType->isArray()) {
    ctx->Error("Type inferred from scope for this expression is " + ctx->highlightError(inferredType->toString()) +
                   ". This expression expects to be of array type",
               fileRange);
  }
  Vec<IR::Value*> valsIR;
  auto*           elemInferredTy = arrTyOfLocal ? arrTyOfLocal->getElementType()
                                                : (inferredType ? inferredType->asArray()->getElementType() : nullptr);
  for (auto* val : values) {
    if ((arrTyOfLocal || isTypeInferred()) && val->hasTypeInferrance() && elemInferredTy) {
      val->asTypeInferrable()->setInferenceType(elemInferredTy);
    }
    valsIR.push_back(val->emit(ctx));
  }
  SHOW("Getting element type")
  IR::QatType* elemTy = nullptr;
  if (!values.empty()) {
    if (valsIR.front()->isReference()) {
      elemTy = valsIR.front()->getType()->asReference()->getSubType();
    } else {
      elemTy = valsIR.front()->getType();
    }
    for (usize i = 1; i < valsIR.size(); i++) {
      auto* val = valsIR.at(i);
      if (val->getType()->isSame(elemTy) ||
          (val->getType()->isReference() && val->getType()->asReference()->getSubType()->isSame(elemTy))) {
        val->loadImplicitPointer(ctx->builder);
        if (val->getType()->isReference()) {
          valsIR.at(i) = new IR::Value(
              ctx->builder.CreateLoad(val->getType()->asReference()->getSubType()->getLLVMType(), val->getLLVM()),
              val->getType()->asReference()->getSubType(), val->getType()->asReference()->isSubtypeVariable(),
              IR::Nature::temporary);
        }
      } else {
        ctx->Error("The expected type is " + ctx->highlightError(elemTy->toString()) +
                       " but the provided expression is of type " + ctx->highlightError(val->getType()->toString()) +
                       ". These do not match",
                   values.at(i)->fileRange);
      }
    }
    // TODO - Implement constant array literals
    llvm::Value* alloca;
    if (isLocalDecl()) {
      if (localValue->getType()->isMaybe()) {
        auto* locll = localValue->getLLVM();
        alloca      = ctx->builder.CreateStructGEP(localValue->getType()->getLLVMType(), locll, 1u);
      } else {
        alloca = localValue->getLLVM();
      }
    } else {
      auto* loc = ctx->getActiveFunction()->getBlock()->newValue(
          irName.has_value() ? irName->value : ctx->getActiveFunction()->getRandomAllocaName(),
          IR::ArrayType::get(elemTy, values.size(), ctx->llctx), isVar, irName.has_value() ? irName->range : fileRange);
      alloca = loc->getAlloca();
    }
    auto* elemPtr =
        ctx->builder.CreateInBoundsGEP(IR::ArrayType::get(elemTy, valsIR.size(), ctx->llctx)->getLLVMType(), alloca,
                                       {llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), 0u),
                                        llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), 0u)});
    ctx->builder.CreateStore(valsIR.at(0)->getLLVM(), elemPtr);
    for (usize i = 1; i < valsIR.size(); i++) {
      elemPtr = ctx->builder.CreateInBoundsGEP(elemTy->getLLVMType(), elemPtr,
                                               llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), 1u));
      SHOW("Value at " << i << " is of type " << valsIR.at(i)->getType()->toString())
      ctx->builder.CreateStore(valsIR.at(i)->getLLVM(), elemPtr);
    }
    return new IR::Value(alloca, IR::ArrayType::get(elemTy, valsIR.size(), ctx->llctx),
                         isLocalDecl() ? localValue->isVariable() : isVar,
                         isLocalDecl() ? localValue->getNature() : IR::Nature::pure);
  } else {
    if (isLocalDecl()) {
      if (localValue->getType()->isArray()) {
        ctx->builder.CreateStore(llvm::ConstantArray::get(llvm::cast<llvm::ArrayType>(arrTyOfLocal->getLLVMType()), {}),
                                 localValue->getLLVM());
      }
    } else {
      if (inferredType->asArray()->getElementType()) {
        return new IR::PrerunValue(
            llvm::ConstantArray::get(llvm::ArrayType::get(inferredType->asArray()->getElementType()->getLLVMType(), 0u),
                                     {}),
            IR::ArrayType::get(inferredType->asArray()->getElementType(), 0u, ctx->llctx));
      } else {
        ctx->Error("Element type for the empty array is not provided and could not be inferred", fileRange);
      }
    }
    return nullptr;
  }
}

Json ArrayLiteral::toJson() const {
  Vec<JsonValue> vals;
  for (auto* exp : values) {
    vals.push_back(exp->toJson());
  }
  return Json()._("nodeType", "arrayLiteral")._("values", vals);
}

} // namespace qat::ast