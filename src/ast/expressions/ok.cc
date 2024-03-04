#include "./ok.hpp"
#include "../../IR/types/result.hpp"

namespace qat::ast {

ir::Value* OkExpression::emit(EmitCtx* ctx) {
  FnAtEnd fnObj{[&] { createIn = nullptr; }};
  if (isTypeInferred()) {
    if (!inferredType->is_result()) {
      ctx->Error("Inferred type is " + ctx->color(inferredType->to_string()) + " cannot be the type of " +
                     ctx->color("ok") + " expression, as it expects a result type",
                 fileRange);
    }
    if (subExpr->hasTypeInferrance()) {
      subExpr->asTypeInferrable()->setInferenceType(inferredType->as_result()->getValidType());
    }
    if (isLocalDecl()) {
      createIn = localValue->to_new_ir_value();
    } else if (canCreateIn()) {
      if (createIn->is_reference() || createIn->is_ghost_pointer()) {
        auto expTy = createIn->is_ghost_pointer() ? createIn->get_ir_type()
                                                  : createIn->get_ir_type()->as_reference()->get_subtype();
        if (!expTy->is_same(inferredType)) {
          ctx->Error("Trying to optimise the ok expression by creating in-place, but the expression type is " +
                         ctx->color(inferredType->to_string()) +
                         " which does not match with the underlying type for in-place creation which is " +
                         ctx->color(expTy->to_string()),
                     fileRange);
        }
      } else {
        ctx->Error(
            "Trying to optimise the ok expression by creating in-place, but the containing type is not a reference",
            fileRange);
      }
    } else {
      createIn =
          ctx->get_fn()->get_block()->new_value(irName.has_value() ? irName->value : utils::unique_id(), inferredType,
                                                isVar, irName.has_value() ? irName->range : fileRange);
    }
    if (subExpr->isInPlaceCreatable()) {
      auto refTy = ir::ReferenceType::get(createIn->get_ir_type()->is_reference()
                                              ? createIn->get_ir_type()->as_reference()->isSubtypeVariable()
                                              : createIn->is_variable(),
                                          inferredType->as_result()->getValidType(), ctx->irCtx);
      subExpr->asInPlaceCreatable()->setCreateIn(ir::Value::get(
          ctx->irCtx->builder.CreatePointerCast(
              ctx->irCtx->builder.CreateStructGEP(inferredType->get_llvm_type(), createIn->get_llvm(), 1u),
              refTy->get_llvm_type()),
          refTy, false));
    }
  } else {
    ctx->Error(ctx->color("ok") + " expression expects the type to be inferred from scope", fileRange);
  }
  auto* expr = subExpr->emit(ctx);
  SHOW("Sub expression emitted")
  if (subExpr->isInPlaceCreatable()) {
    subExpr->asInPlaceCreatable()->unsetCreateIn();
    ctx->irCtx->builder.CreateStore(
        llvm::ConstantInt::getTrue(llvm::Type::getInt1Ty(ctx->irCtx->llctx)),
        ctx->irCtx->builder.CreateStructGEP(inferredType->get_llvm_type(), createIn->get_llvm(), 0u));
    return createIn;
  }
  if (isTypeInferred() &&
      !(expr->get_ir_type()->is_same(inferredType->as_result()->getValidType()) ||
        (expr->get_ir_type()->is_reference() &&
         expr->get_ir_type()->as_reference()->get_subtype()->is_same(inferredType->as_result()->getValidType())))) {
    ctx->Error("Provided expression is of incompatible type " + ctx->color(expr->get_ir_type()->to_string()) +
                   ". Expected an expression of type " +
                   ctx->color(inferredType->as_result()->getValidType()->to_string()),
               subExpr->fileRange);
  }
  if ((expr->get_ir_type()->is_same(inferredType->as_result()->getValidType()) && expr->is_ghost_pointer()) ||
      expr->get_ir_type()->is_reference()) {
    if (!expr->get_ir_type()->is_same(inferredType->as_result()->getValidType())) {
      expr->load_ghost_pointer(ctx->irCtx->builder);
    }
    auto validTy = inferredType->as_result()->getValidType();
    if (validTy->is_trivially_copyable() || validTy->is_trivially_movable()) {
      ctx->irCtx->builder.CreateStore(
          ctx->irCtx->builder.CreateLoad(validTy->get_llvm_type(), expr->get_llvm()),
          ctx->irCtx->builder.CreatePointerCast(
              ctx->irCtx->builder.CreateStructGEP(inferredType->get_llvm_type(), createIn->get_llvm(), 1u),
              llvm::PointerType::get(validTy->get_llvm_type(), ctx->irCtx->dataLayout->getProgramAddressSpace())));
      ctx->irCtx->builder.CreateStore(
          llvm::ConstantInt::getTrue(llvm::Type::getInt1Ty(ctx->irCtx->llctx)),
          ctx->irCtx->builder.CreateStructGEP(inferredType->get_llvm_type(), createIn->get_llvm(), 0u));
      if (!validTy->is_trivially_copyable()) {
        if (expr->get_ir_type()->is_same(inferredType->as_result()->getValidType())) {
          if (!expr->is_variable()) {
            ctx->Error("This is an expression without variability and hence cannot be moved from", fileRange);
          }
        } else if (!expr->get_ir_type()->as_reference()->isSubtypeVariable()) {
          ctx->Error("This expression is of type " + ctx->color(expr->get_ir_type()->to_string()) +
                         " which is a reference without variability and hence cannot be trivially moved from",
                     fileRange);
        }
        // MOVE WARNING
        ctx->irCtx->Warning("There is a trivial move occuring here. Do you want to use " +
                                ctx->irCtx->highlightWarning("'move") + " to make it explicit and clear?",
                            subExpr->fileRange);
        ctx->irCtx->builder.CreateStore(llvm::ConstantExpr::getNullValue(validTy->get_llvm_type()), expr->get_llvm());
        if (expr->is_local_value()) {
          ctx->get_fn()->get_block()->addMovedValue(expr->get_local_id().value());
        }
      }
      return createIn;
    } else {
      // NON-TRIVIAL COPY & MOVE ERROR
      ctx->Error("The expression provided is of type " + ctx->color(expr->get_ir_type()->to_string()) + ". Type " +
                     ctx->color(validTy->to_string()) + " cannot be trivially copied or moved. Please do " +
                     ctx->color("'copy") + " or " + ctx->color("'move") + " accordingly",
                 subExpr->fileRange);
    }
  } else if (expr->get_ir_type()->is_same(inferredType->as_result()->getValidType())) {
    auto validTy = inferredType->as_result()->getValidType();
    ctx->irCtx->builder.CreateStore(
        expr->get_llvm(),
        ctx->irCtx->builder.CreatePointerCast(
            ctx->irCtx->builder.CreateStructGEP(inferredType->get_llvm_type(), createIn->get_llvm(), 1u),
            llvm::PointerType::get(validTy->get_llvm_type(), ctx->irCtx->dataLayout->getProgramAddressSpace())));
    ctx->irCtx->builder.CreateStore(
        llvm::ConstantInt::getTrue(llvm::Type::getInt1Ty(ctx->irCtx->llctx)),
        ctx->irCtx->builder.CreateStructGEP(inferredType->get_llvm_type(), createIn->get_llvm(), 0u));
  }
  return createIn;
}

Json OkExpression::to_json() const {
  return Json()._("nodeType", "ok")._("hasSubExpression", subExpr != nullptr)._("fileRange", fileRange);
}

} // namespace qat::ast