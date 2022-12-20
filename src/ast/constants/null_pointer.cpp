#include "./null_pointer.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/void.hpp"
#include "llvm/IR/Constants.h"

namespace qat::ast {

NullPointer::NullPointer(FileRange _fileRange) : ConstantExpression(std::move(_fileRange)) {}

void NullPointer::setType(IR::PointerType* typ) { candidateType = typ; }

IR::ConstantValue* NullPointer::emit(IR::Context* ctx) {
  if (getExpectedKind() == ExpressionKind::assignable) {
    ctx->Error("Null pointer is not assignable", fileRange);
  }
  SHOW("Null pointer has type provided " << (candidateType != nullptr))
  if (candidateType) {
    return new IR::ConstantValue(
        candidateType->isMulti()
            ? llvm::ConstantStruct::get(llvm::dyn_cast<llvm::StructType>(candidateType->getLLVMType()),
                                        {llvm::ConstantPointerNull::get(candidateType->getLLVMType()->getPointerTo()),
                                         llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), 0u)})
            : llvm::ConstantPointerNull::get(candidateType->getLLVMType()->getPointerTo()),
        IR::PointerType::get(candidateType->isSubtypeVariable(), candidateType->getSubType(), candidateType->getOwner(),
                             candidateType->isMulti(), ctx->llctx));
  } else {
    return new IR::ConstantValue(
        llvm::ConstantPointerNull::get(llvm::Type::getInt8Ty(ctx->llctx)->getPointerTo()),
        IR::PointerType::get(false, IR::VoidType::get(ctx->llctx), IR::PointerOwner::OfAnonymous(), false, ctx->llctx));
  }
}

Json NullPointer::toJson() const { return Json()._("nodeType", "nullPointer"); }

} // namespace qat::ast