#include "./member_access.hpp"
#include "llvm/Analysis/ConstantFolding.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"

namespace qat::ast {

ir::PrerunValue* PrerunMemberAccess::emit(EmitCtx* ctx) {
  auto* irExp = expr->emit(ctx);
  if (irExp->get_ir_type()->is_maybe()) {
    if (memberName.value == "hasValue") {
      return ir::PrerunValue::get(irExp->get_llvm_constant()->getAggregateElement(0u),
                                  ir::UnsignedType::getBool(ctx->irCtx));
    } else if (memberName.value == "hasNoValue") {
      return ir::PrerunValue::get(
          llvm::ConstantFoldConstant(
              llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_EQ,
                                          irExp->get_llvm_constant()->getAggregateElement(0u),
                                          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), 0u)),
              ctx->irCtx->dataLayout.value()),
          ir::UnsignedType::getBool(ctx->irCtx));
    } else if (memberName.value == "get") {
      if (llvm::cast<llvm::ConstantInt>(
              llvm::cast<llvm::ConstantStruct>(irExp->get_llvm_constant())->getAggregateElement(0u))
              ->getValue()
              .getBoolValue()) {
      } else {
        ctx->Error("This maybe expression does not contain any value, so the underlying value cannot be obtained",
                   fileRange);
      }
    } else {
      ctx->Error("Invalid name for member access for expression of maybe type " +
                     ctx->color(irExp->get_ir_type()->to_string()),
                 memberName.range);
    }
  } else if (irExp->get_ir_type()->is_array()) {
    if (memberName.value == "length") {
      return ir::PrerunValue::get(llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->irCtx->llctx),
                                                         irExp->get_ir_type()->as_array()->get_length()),
                                  ir::UnsignedType::get(64u, ctx->irCtx));
    } else {
      ctx->Error("Invalid name for member access for expression of array type " +
                     ctx->color(irExp->get_ir_type()->to_string()),
                 fileRange);
    }
  } else {
    ctx->Error("The expression is of type " + ctx->color(irExp->get_ir_type()->to_string()) +
                   " and cannot use member access",
               expr->fileRange);
  }
  return nullptr;
}

Json PrerunMemberAccess::to_json() const {
  return Json()
      ._("nodeType", "prerunMemberAccess")
      ._("expression", expr->to_json())
      ._("memberName", memberName)
      ._("fileRange", fileRange);
}

String PrerunMemberAccess::to_string() const { return expr->to_string() + "'" + memberName.value; }

} // namespace qat::ast