#include "./null_pointer.hpp"
#include "../../IR/types/void.hpp"
#include "llvm/IR/Constants.h"

namespace qat::ast {

NullPointer::NullPointer(utils::FileRange _fileRange) : ConstantExpression(std::move(_fileRange)) {}

void NullPointer::setType(bool isVariable, IR::QatType* typ) {
  isVariableType = isVariable;
  candidateType  = typ;
}

IR::ConstantValue* NullPointer::emit(IR::Context* ctx) {
  if (getExpectedKind() == ExpressionKind::assignable) {
    ctx->Error("Null pointer is not assignable", fileRange);
  }
  if (candidateType) {
    return new IR::ConstantValue(llvm::ConstantPointerNull::get(candidateType->getLLVMType()->getPointerTo()),
                                 IR::PointerType::get(isVariableType, candidateType, ctx->llctx));
  } else {
    return new IR::ConstantValue(llvm::ConstantPointerNull::get(llvm::Type::getInt8Ty(ctx->llctx)->getPointerTo()),
                                 IR::PointerType::get(isVariableType, IR::VoidType::get(ctx->llctx), ctx->llctx));
  }
}

Json NullPointer::toJson() const { return Json()._("nodeType", "nullPointer"); }

} // namespace qat::ast