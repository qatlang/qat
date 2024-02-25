#include "./member_access.hpp"
#include "llvm/Analysis/ConstantFolding.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"

namespace qat::ast {

IR::PrerunValue* PrerunMemberAccess::emit(IR::Context* ctx) {
  auto* irExp = expr->emit(ctx);
  if (irExp->getType()->isMaybe()) {
    if (memberName.value == "hasValue") {
      return new IR::PrerunValue(irExp->getLLVMConstant()->getAggregateElement(0u), IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "hasNoValue") {
      return new IR::PrerunValue(
          llvm::ConstantFoldConstant(
              llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_EQ,
                                          irExp->getLLVMConstant()->getAggregateElement(0u),
                                          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 0u)),
              ctx->dataLayout.value()),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "get") {
      if (llvm::cast<llvm::ConstantInt>(
              llvm::cast<llvm::ConstantStruct>(irExp->getLLVMConstant())->getAggregateElement(0u))
              ->getValue()
              .getBoolValue()) {
      } else {
        ctx->Error("This maybe expression does not contain any value, so the underlying value cannot be obtained",
                   fileRange);
      }
    } else {
      ctx->Error("Invalid name for member access for expression of maybe type " +
                     ctx->highlightError(irExp->getType()->toString()),
                 memberName.range);
    }
  } else if (irExp->getType()->isArray()) {
    if (memberName.value == "length") {
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), irExp->getType()->asArray()->getLength()),
          IR::UnsignedType::get(64u, ctx));
    } else {
      ctx->Error("Invalid name for member access for expression of array type " +
                     ctx->highlightError(irExp->getType()->toString()),
                 fileRange);
    }
  } else {
    ctx->Error("The expression is of type " + ctx->highlightError(irExp->getType()->toString()) +
                   " and cannot use member access",
               expr->fileRange);
  }
  return nullptr;
}

Json PrerunMemberAccess::toJson() const {
  return Json()
      ._("nodeType", "prerunMemberAccess")
      ._("expression", expr->toJson())
      ._("memberName", memberName)
      ._("fileRange", fileRange);
}

String PrerunMemberAccess::toString() const { return expr->toString() + "'" + memberName.value; }

} // namespace qat::ast