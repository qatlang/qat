#include "./null_pointer.hpp"
#include "../../IR/logic.hpp"
#include "llvm/IR/Constants.h"

namespace qat::ast {

ir::PrerunValue* NullPointer::emit(EmitCtx* ctx) {
  if (!providedType.has_value() && !isTypeInferred()) {
    ctx->Error("No type provided for null pointer and no type inferred from scope", fileRange);
  }
  auto theType = providedType.has_value() ? providedType.value()->emit(ctx) : inferredType;
  if (providedType.has_value() && isTypeInferred()) {
    if (!theType->is_same(inferredType)) {
      ctx->Error("The provided type for the null pointer is " + ctx->color(theType->to_string()) +
                     ", but the inferred type is " + ctx->color(inferredType->to_string()),
                 fileRange);
    }
  }
  ir::Type* finalTy = theType;
  if (theType->is_pointer() || (theType->is_ctype() && theType->as_ctype()->get_subtype()->is_pointer())) {
    if (theType->is_ctype() ? theType->as_ctype()->get_subtype()->as_pointer()->isNonNullable()
                            : theType->as_pointer()->isNonNullable()) {
      ctx->Error("The inferred type is " + ctx->color(theType->to_string()) + " which is not a nullable pointer type",
                 fileRange);
    }
    if (theType->is_ctype()) {
      finalTy = theType->as_ctype()->get_subtype();
    }
  } else {
    ctx->Error("The inferred type for null is " + ctx->color(theType->to_string()) + " which is not a pointer type",
               fileRange);
  }
  auto llPtrTy = llvm::PointerType::get(
      llvm::PointerType::isValidElementType(finalTy->as_pointer()->get_subtype()->get_llvm_type())
          ? finalTy->as_pointer()->get_subtype()->get_llvm_type()
          : llvm::Type::getInt8Ty(ctx->irCtx->llctx),
      ctx->irCtx->dataLayout->getProgramAddressSpace());
  return ir::PrerunValue::get(
      finalTy->as_pointer()->isMulti()
          ? llvm::ConstantStruct::get(llvm::dyn_cast<llvm::StructType>(finalTy->get_llvm_type()),
                                      {llvm::ConstantPointerNull::get(llPtrTy),
                                       llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->irCtx->llctx), 0u)})
          : llvm::ConstantPointerNull::get(llPtrTy),
      theType);
}

String NullPointer::to_string() const {
  return "null" + (providedType.has_value() ? (":[" + providedType.value()->to_string() + "]") : "");
}

Json NullPointer::to_json() const { return Json()._("nodeType", "nullPointer")._("fileRange", fileRange); }

} // namespace qat::ast