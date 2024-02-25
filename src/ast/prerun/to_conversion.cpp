#include "./to_conversion.hpp"
#include "llvm/IR/Constants.h"

namespace qat::ast {

void PrerunTo::update_dependencies(IR::EmitPhase phase, Maybe<IR::DependType> dep, IR::EntityState* ent,
                                   IR::Context* ctx) {
  value->update_dependencies(phase, IR::DependType::complete, ent, ctx);
  targetType->update_dependencies(phase, IR::DependType::complete, ent, ctx);
}

IR::PrerunValue* PrerunTo::emit(IR::Context* ctx) {
  auto val          = value->emit(ctx);
  auto usableTarget = targetType->emit(ctx);
  auto target       = usableTarget->isCType() ? usableTarget->asCType()->getSubType() : usableTarget;
  auto valTy        = val->getType();
  if (valTy->isCType()) {
    valTy = valTy->asCType()->getSubType();
  }
  if (valTy->isSame(target)) {
    return val;
  }
  if (valTy->isInteger()) {
    if (target->isInteger()) {
      return new IR::PrerunValue(
          llvm::ConstantExpr::getIntegerCast(val->getLLVMConstant(), target->getLLVMType(), true), usableTarget);
    } else if (target->isUnsignedInteger()) {
      return new IR::PrerunValue(
          llvm::ConstantExpr::getIntegerCast(val->getLLVMConstant(), target->getLLVMType(), false), usableTarget);
    } else if (target->isFloat()) {
      return new IR::PrerunValue(llvm::ConstantExpr::getFPCast(val->getLLVMConstant(), target->getLLVMType()),
                                 usableTarget);
    }
  } else if (valTy->isUnsignedInteger()) {
    if (target->isInteger()) {
      return new IR::PrerunValue(
          llvm::ConstantExpr::getIntegerCast(val->getLLVMConstant(), target->getLLVMType(), true), usableTarget);
    } else if (target->isUnsignedInteger()) {
      return new IR::PrerunValue(
          llvm::ConstantExpr::getIntegerCast(val->getLLVMConstant(), target->getLLVMType(), false), usableTarget);
    } else if (target->isFloat()) {
      return new IR::PrerunValue(llvm::ConstantExpr::getFPCast(val->getLLVMConstant(), target->getLLVMType()),
                                 usableTarget);
    }
  } else if (valTy->isFloat()) {
    if (target->isInteger()) {
      return new IR::PrerunValue(llvm::ConstantExpr::getFPToSI(val->getLLVMConstant(), target->getLLVMType()),
                                 usableTarget);
    } else if (target->isUnsignedInteger()) {
      return new IR::PrerunValue(llvm::ConstantExpr::getFPToUI(val->getLLVMConstant(), target->getLLVMType()),
                                 usableTarget);
    } else if (target->isFloat()) {
      return new IR::PrerunValue(llvm::ConstantExpr::getFPCast(val->getLLVMConstant(), target->getLLVMType()),
                                 usableTarget);
    }
  } else if (valTy->isStringSlice()) {
    if (usableTarget->isCType() && usableTarget->asCType()->isCString()) {
      return new IR::PrerunValue(val->getLLVMConstant()->getAggregateElement(0u), usableTarget);
    } else if (valTy->isPointer() &&
               (valTy->asPointer()->getSubType()->isUnsignedInteger() ||
                (valTy->asPointer()->getSubType()->isCType() &&
                 valTy->asPointer()->getSubType()->asCType()->getSubType()->isUnsignedInteger())) &&
               (valTy->asPointer()->getSubType()->isUnsignedInteger()
                    ? (valTy->asPointer()->getSubType()->asUnsignedInteger()->getBitwidth() == 8u)
                    : (valTy->asPointer()->getSubType()->asCType()->getSubType()->asUnsignedInteger()->getBitwidth() ==
                       8u)) &&
               (valTy->asPointer()->getOwner().isAnonymous()) && (!valTy->asPointer()->isSubtypeVariable())) {
      if (valTy->asPointer()->isMulti()) {
        return new IR::PrerunValue(llvm::ConstantExpr::getBitCast(val->getLLVMConstant(), usableTarget->getLLVMType()),
                                   usableTarget);
      } else {
        return new IR::PrerunValue(val->getLLVMConstant()->getAggregateElement(0u), usableTarget);
      }
    }
  } else if (valTy->isChoice()) {
    if ((target->isInteger() || target->isUnsignedInteger()) && valTy->asChoice()->hasProvidedType() &&
        (target->isInteger() == valTy->asChoice()->getProvidedType()->isInteger())) {
      auto* intTy = valTy->asChoice()->getProvidedType()->asInteger();
      if (intTy->getBitwidth() == target->asInteger()->getBitwidth()) {
        return new IR::PrerunValue(val->getLLVM(), usableTarget);
      } else {
        ctx->Error("The underlying type of the choice type " + ctx->highlightError(valTy->toString()) + " is " +
                       ctx->highlightError(intTy->toString()) + ", but the type for conversion is " +
                       ctx->highlightError(target->toString()),
                   fileRange);
      }
    } else {
      ctx->Error("The underlying type of the choice type " + ctx->highlightError(valTy->toString()) + " is " +
                     ctx->highlightError(valTy->asChoice()->getProvidedType()->toString()) +
                     ", but the target type for conversion is " + ctx->highlightError(target->toString()),
                 fileRange);
    }
  }
  ctx->Error("Conversion from " + ctx->highlightError(val->getType()->toString()) + " to " +
                 ctx->highlightError(usableTarget->toString()) + " is not supported. Try casting instead",
             fileRange);
}

String PrerunTo::toString() const { return value->toString() + " to " + targetType->toString(); }

Json PrerunTo::toJson() const {
  return Json()
      ._("nodeType", "prerunToConversion")
      ._("value", value->toJson())
      ._("targetType", targetType->toJson())
      ._("fileRange", fileRange);
}

} // namespace qat::ast