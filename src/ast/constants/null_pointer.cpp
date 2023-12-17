#include "./null_pointer.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/void.hpp"
#include "llvm/IR/Constants.h"

namespace qat::ast {

NullPointer::NullPointer(FileRange _fileRange) : PrerunExpression(std::move(_fileRange)) {}

IR::PrerunValue* NullPointer::emit(IR::Context* ctx) {
  if (isTypeInferred()) {
    if (inferredType->isPointer()) {
      if (!inferredType->asPointer()->isNullable()) {
        ctx->Error("The inferred type is " + ctx->highlightError(inferredType->toString()) +
                       " which is not a nullable pointer type",
                   fileRange);
      }
    } else {
      ctx->Error("The inferred type for null is " + ctx->highlightError(inferredType->toString()) +
                     " which is not a pointer type",
                 fileRange);
    }
    return new IR::PrerunValue(
        inferredType->asPointer()->isMulti()
            ? llvm::ConstantStruct::get(
                  llvm::dyn_cast<llvm::StructType>(inferredType->getLLVMType()),
                  {llvm::ConstantPointerNull::get(inferredType->asPointer()->getSubType()->getLLVMType()->getPointerTo(
                       ctx->dataLayout->getProgramAddressSpace())),
                   llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), 0u)})
            : llvm::ConstantPointerNull::get(inferredType->asPointer()->getSubType()->getLLVMType()->getPointerTo()),
        IR::PointerType::get(inferredType->asPointer()->isSubtypeVariable(), inferredType->asPointer()->getSubType(),
                             false, inferredType->asPointer()->getOwner(), inferredType->asPointer()->isMulti(), ctx));
  } else {
    return new IR::PrerunValue(
        llvm::ConstantPointerNull::get(llvm::Type::getInt8Ty(ctx->llctx)->getPointerTo()),
        IR::PointerType::get(false, IR::VoidType::get(ctx->llctx), false, IR::PointerOwner::OfAnonymous(), false, ctx));
  }
}

String NullPointer::toString() const { return "null"; }

Json NullPointer::toJson() const { return Json()._("nodeType", "nullPointer")._("fileRange", fileRange); }

} // namespace qat::ast