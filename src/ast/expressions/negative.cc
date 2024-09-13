#include "./negative.hpp"
#include "llvm/Analysis/ConstantFolding.h"

namespace qat::ast {

ir::Value* Negative::emit(EmitCtx* ctx) {
  if (is_type_inferred() && value->has_type_inferrance()) {
    value->as_type_inferrable()->set_inference_type(inferredType);
  }
  auto irVal = value->emit(ctx);
  auto valTy =
      irVal->get_ir_type()->is_reference() ? irVal->get_ir_type()->as_reference()->get_subtype() : irVal->get_ir_type();
  auto typeCheck = [&](ir::Type* candTy) {
    if (is_type_inferred()) {
      if (!candTy->is_same(inferredType)) {
        ctx->Error("The expression is of type " + ctx->color(candTy->to_string()) +
                       ", but the type inferred from scope is " + ctx->color(inferredType->to_string()),
                   value->fileRange);
      }
    }
  };
  if (valTy->is_integer() || (valTy->is_ctype() && valTy->as_ctype()->get_subtype()->is_integer())) {
    typeCheck(valTy);
    if (irVal->is_prerun_value()) {
      return ir::PrerunValue::get(llvm::ConstantExpr::getNeg(irVal->get_llvm_constant()), valTy);
    } else {
      irVal->load_ghost_reference(ctx->irCtx->builder);
      if (irVal->get_ir_type()->is_reference()) {
        irVal = ir::Value::get(ctx->irCtx->builder.CreateLoad(valTy->get_llvm_type(), irVal->get_llvm()), valTy, false);
      }
      return ir::Value::get(ctx->irCtx->builder.CreateNeg(irVal->get_llvm()), valTy, false);
    }
  } else if (valTy->is_float() || (valTy->is_ctype() && valTy->as_ctype()->get_subtype()->is_float())) {
    typeCheck(valTy);
    if (irVal->is_prerun_value()) {
      return ir::PrerunValue::get(
          llvm::cast<llvm::Constant>(ctx->irCtx->builder.CreateFNeg(irVal->get_llvm_constant())), valTy);
    } else {
      irVal->load_ghost_reference(ctx->irCtx->builder);
      if (irVal->get_ir_type()->is_reference()) {
        irVal = ir::Value::get(ctx->irCtx->builder.CreateLoad(valTy->get_llvm_type(), irVal->get_llvm()), valTy, false);
      }
      return ir::Value::get(
          ctx->irCtx->builder.CreateFSub(llvm::ConstantFP::getZero(valTy->get_llvm_type()), irVal->get_llvm()), valTy,
          false);
    }
  } else if (valTy->is_unsigned_integer() ||
             (valTy->is_ctype() && valTy->as_ctype()->get_subtype()->is_unsigned_integer())) {
    ctx->Error("The value of this expression is of the unsigned integer type " + ctx->color(valTy->to_string()) +
                   " and cannot use the " + ctx->color("unary -") + " operator",
               fileRange);
  } else if (valTy->is_expanded()) {
    // FIXME - Prerun expanded type values
    if (valTy->as_expanded()->has_unary_operator("-")) {
      auto localID = irVal->get_local_id();
      if (irVal->get_ir_type()->is_reference() || irVal->is_ghost_reference()) {
        if (irVal->get_ir_type()->is_reference()) {
          irVal->load_ghost_reference(ctx->irCtx->builder);
        }
      } else {
        auto* loc = ctx->get_fn()->get_block()->new_value(utils::unique_id(), valTy, true, fileRange);
        ctx->irCtx->builder.CreateStore(irVal->get_llvm(), loc->get_llvm());
        irVal = loc;
      }
      auto opFn   = valTy->as_expanded()->get_unary_operator("-");
      auto result = opFn->call(ctx->irCtx, {irVal->get_llvm()}, localID, ctx->mod);
      typeCheck(result->get_ir_type());
      return result;
    } else {
      ctx->Error("Type " + ctx->color(valTy->to_string()) + " does not have the " + ctx->color("unary -") + " operator",
                 fileRange);
    }
  } else {
    // FIXME - Add default skill support
    ctx->Error("Type " + ctx->color(valTy->to_string()) + " does not support the " + ctx->color("unary -") +
                   " operator",
               fileRange);
  }
}

Json Negative::to_json() const {
  return Json()._("nodeType", "negative")._("subExpression", value->to_json())._("fileRange", fileRange);
}

} // namespace qat::ast
