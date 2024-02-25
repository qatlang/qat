#include "./null_pointer.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/void.hpp"
#include "llvm/IR/Constants.h"

namespace qat::ast {

IR::PrerunValue* NullPointer::emit(IR::Context* ctx) {
  if (!providedType.has_value() && !isTypeInferred()) {
    ctx->Error("No type provided for null pointer and no type inferred from scope", fileRange);
  }
  auto theType = providedType.has_value() ? providedType.value()->emit(ctx) : inferredType;
  if (providedType.has_value() && isTypeInferred()) {
    if (!theType->isSame(inferredType)) {
      ctx->Error("The provided type for the null pointer is " + ctx->highlightError(theType->toString()) +
                     ", but the inferred type is " + ctx->highlightError(inferredType->toString()),
                 fileRange);
    }
  }
  IR::QatType* finalTy = theType;
  if (theType->isPointer() || (theType->isCType() && theType->asCType()->getSubType()->isPointer())) {
    if (theType->isCType() ? theType->asCType()->getSubType()->asPointer()->isNonNullable()
                           : theType->asPointer()->isNonNullable()) {
      ctx->Error("The inferred type is " + ctx->highlightError(theType->toString()) +
                     " which is not a nullable pointer type",
                 fileRange);
    }
    if (theType->isCType()) {
      finalTy = theType->asCType()->getSubType();
    }
  } else {
    ctx->Error("The inferred type for null is " + ctx->highlightError(theType->toString()) +
                   " which is not a pointer type",
               fileRange);
  }
  auto llPtrTy =
      llvm::PointerType::get(llvm::PointerType::isValidElementType(finalTy->asPointer()->getSubType()->getLLVMType())
                                 ? finalTy->asPointer()->getSubType()->getLLVMType()
                                 : llvm::Type::getInt8Ty(ctx->llctx),
                             ctx->dataLayout->getProgramAddressSpace());
  return new IR::PrerunValue(
      finalTy->asPointer()->isMulti()
          ? llvm::ConstantStruct::get(llvm::dyn_cast<llvm::StructType>(finalTy->getLLVMType()),
                                      {llvm::ConstantPointerNull::get(llPtrTy),
                                       llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), 0u)})
          : llvm::ConstantPointerNull::get(llPtrTy),
      theType);
}

String NullPointer::toString() const {
  return "null" + (providedType.has_value() ? (":[" + providedType.value()->toString() + "]") : "");
}

Json NullPointer::toJson() const { return Json()._("nodeType", "nullPointer")._("fileRange", fileRange); }

} // namespace qat::ast