#include "./null_pointer.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/void.hpp"
#include "llvm/IR/Constants.h"

namespace qat::ast {

NullPointer::NullPointer(FileRange _fileRange) : ConstantExpression(std::move(_fileRange)) {}

IR::ConstantValue* NullPointer::emit(IR::Context* ctx) {
  if (getExpectedKind() == ExpressionKind::assignable) {
    ctx->Error("Null pointer is not assignable", fileRange);
  }
  if (inferredType) {
    return new IR::ConstantValue(
        inferredType.value()->asPointer()->isMulti()
            ? llvm::ConstantStruct::get(
                  llvm::dyn_cast<llvm::StructType>(inferredType.value()->getLLVMType()),
                  {llvm::ConstantPointerNull::get(
                       inferredType.value()->asPointer()->getSubType()->getLLVMType()->getPointerTo(
                           ctx->dataLayout->getProgramAddressSpace())),
                   llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), 0u)})
            : llvm::ConstantPointerNull::get(
                  inferredType.value()->asPointer()->getSubType()->getLLVMType()->getPointerTo()),
        IR::PointerType::get(
            inferredType.value()->asPointer()->isSubtypeVariable(), inferredType.value()->asPointer()->getSubType(),
            inferredType.value()->asPointer()->getOwner(), inferredType.value()->asPointer()->isMulti(), ctx));
  } else {
    return new IR::ConstantValue(
        llvm::ConstantPointerNull::get(llvm::Type::getInt8Ty(ctx->llctx)->getPointerTo()),
        IR::PointerType::get(false, IR::VoidType::get(ctx->llctx), IR::PointerOwner::OfAnonymous(), false, ctx));
  }
}

String NullPointer::toString() const { return "null"; }

Json NullPointer::toJson() const { return Json()._("nodeType", "nullPointer"); }

} // namespace qat::ast