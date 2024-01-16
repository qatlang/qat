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
  if (theType->isPointer()) {
    if (!theType->asPointer()->isNullable()) {
      ctx->Error("The inferred type is " + ctx->highlightError(theType->toString()) +
                     " which is not a nullable pointer type",
                 fileRange);
    }
  } else {
    ctx->Error("The inferred type for null is " + ctx->highlightError(theType->toString()) +
                   " which is not a pointer type",
               fileRange);
  }
  return new IR::PrerunValue(
      theType->asPointer()->isMulti()
          ? llvm::ConstantStruct::get(
                llvm::dyn_cast<llvm::StructType>(theType->getLLVMType()),
                {llvm::ConstantPointerNull::get(theType->asPointer()->getSubType()->getLLVMType()->getPointerTo(
                     ctx->dataLayout->getProgramAddressSpace())),
                 llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), 0u)})
          : llvm::ConstantPointerNull::get(theType->asPointer()->getSubType()->getLLVMType()->getPointerTo()),
      IR::PointerType::get(theType->asPointer()->isSubtypeVariable(), theType->asPointer()->getSubType(), false,
                           theType->asPointer()->getOwner(), theType->asPointer()->isMulti(), ctx));
}

String NullPointer::toString() const { return "null"; }

Json NullPointer::toJson() const { return Json()._("nodeType", "nullPointer")._("fileRange", fileRange); }

} // namespace qat::ast