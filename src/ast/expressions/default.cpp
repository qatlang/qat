#include "./default.hpp"
#include "../../IR/types/maybe.hpp"

namespace qat::ast {

IR::Value* Default::emit(IR::Context* ctx) {
  auto theType = providedType.has_value() ? providedType.value()->emit(ctx) : inferredType;
  if (theType) {
    if (theType->isInteger()) {
      if (isLocalDecl()) {
        ctx->builder.CreateStore(llvm::ConstantInt::get(theType->getLLVMType(), 0u, true), localValue->getAlloca());
        return nullptr;
      } else if (irName.has_value()) {
        auto* block = ctx->getActiveFunction()->getBlock();
        auto* loc   = block->newValue(irName->value, theType, isVar, irName->range);
        ctx->builder.CreateStore(llvm::ConstantInt::get(theType->getLLVMType(), 0u, true), loc->getAlloca());
        return loc->toNewIRValue();
      } else {
        return new IR::PrerunValue(llvm::ConstantInt::get(theType->asInteger()->getLLVMType(), 0u, true), theType);
      }
    } else if (theType->isUnsignedInteger()) {
      if (isLocalDecl()) {
        ctx->builder.CreateStore(llvm::ConstantInt::get(theType->getLLVMType(), 0u), localValue->getAlloca());
        return nullptr;
      } else if (irName.has_value()) {
        auto* loc = ctx->getActiveFunction()->getBlock()->newValue(irName->value, theType, isVar, irName->range);
        ctx->builder.CreateStore(llvm::ConstantInt::get(theType->getLLVMType(), 0u), loc->getAlloca());
        return loc->toNewIRValue();
      } else {
        return new IR::PrerunValue(llvm::ConstantInt::get(theType->asUnsignedInteger()->getLLVMType(), 0u), theType);
      }
    } else if (theType->isPointer()) {
      if (theType->asPointer()->isNullable()) {
        return new IR::PrerunValue(llvm::ConstantExpr::getNullValue(theType->getLLVMType()), theType);
      } else {
        ctx->Error("The pointer type is " + ctx->highlightError(theType->toString()) +
                       " which is not nullable, and hence cannot have a default value",
                   fileRange);
      }
    } else if (theType->isReference()) {
      ctx->Error("Cannot get default value for a reference type", fileRange);
    } else if (theType->isExpanded()) {
      auto* eTy = theType->asExpanded();
      if (eTy->hasDefaultConstructor()) {
        auto* defFn = eTy->getDefaultConstructor();
        if (!defFn->isAccessible(ctx->getAccessInfo())) {
          ctx->Error("The default constructor of type " + ctx->highlightError(eTy->toString()) +
                         " is not accessible here",
                     fileRange);
        }
        auto* block = ctx->getActiveFunction()->getBlock();
        if (isLocalDecl()) {
          (void)defFn->call(ctx, {localValue->getAlloca()}, None, ctx->getMod());
          return nullptr;
        } else {
          auto* loc = block->newValue(irName.has_value() ? irName->value : utils::unique_id(), eTy, true, fileRange);
          (void)defFn->call(ctx, {loc->getAlloca()}, None, ctx->getMod());
          return loc->toNewIRValue();
        }
      } else {
        ctx->Error("Type " + ctx->highlightError(eTy->getFullName()) +
                       " does not have a default constructor. Please check logic and make necessary changes",
                   fileRange);
      }
    } else if (theType->isMaybe()) {
      auto* mTy   = theType->asMaybe();
      auto* block = ctx->getActiveFunction()->getBlock();
      if (isLocalDecl()) {
        ctx->builder.CreateStore(llvm::Constant::getNullValue(mTy->getLLVMType()), localValue->getLLVM());
        return nullptr;
      } else {
        // FIXME - Change after adding checks if type can be prerun
        auto* loc = block->newValue(irName.has_value() ? irName->value : utils::unique_id(), mTy, true,
                                    irName.has_value() ? irName->range : fileRange);
        ctx->builder.CreateStore(llvm::Constant::getNullValue(mTy->getLLVMType()), loc->getLLVM());
        return loc->toNewIRValue();
      }
    } else if (theType->isChoice()) {
      auto* chTy = theType->asChoice();
      if (chTy->hasDefault()) {
        return new IR::PrerunValue(chTy->getDefault(), theType);
      } else {
        ctx->Error("Choice type " + ctx->highlightError(theType->toString()) + " does not have a default value",
                   fileRange);
      }
    } else {
      ctx->Error("The type " + ctx->highlightError(theType->toString()) + " does not have default value", fileRange);
      // FIXME - Handle other types
    }
  } else {
    ctx->Error("default has no type inferred from scope, and there is no type provided like " +
                   ctx->highlightError("default:[CustomType]"),
               fileRange);
  }
}

Json Default::toJson() const {
  return Json()
      ._("nodeType", "default")
      ._("hasProvidedType", providedType.has_value())
      ._("providedType", (providedType.has_value() ? providedType.value()->toJson() : JsonValue()));
}

} // namespace qat::ast