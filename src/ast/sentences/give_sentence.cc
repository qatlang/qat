#include "./give_sentence.hpp"
#include "../../IR/types/void.hpp"

namespace qat::ast {

ir::Value* GiveSentence::emit(EmitCtx* ctx) {
  auto* fun = ctx->get_fn();
  if (give_expr.has_value()) {
    if (fun->get_ir_type()->as_function()->get_return_type()->is_return_self()) {
      if (give_expr.value()->nodeType() != NodeType::SELF) {
        ctx->Error("This function is marked to return '' and cannot return anything else", fileRange);
      }
    }
    auto* retType = fun->get_ir_type()->as_function()->get_return_type()->get_type();
    if (give_expr.value()->hasTypeInferrance()) {
      give_expr.value()->asTypeInferrable()->setInferenceType(retType);
    }
    auto* retVal = give_expr.value()->emit(ctx);
    SHOW("RetType: " << retType->to_string() << "\nRetValType: " << retVal->getType()->to_string())
    if (retType->is_same(retVal->get_ir_type())) {
      if (retType->is_void()) {
        ir::destructorCaller(ctx->irCtx, fun);
        ir::memberFunctionHandler(ctx->irCtx, fun);
        return ir::Value::get(ctx->irCtx->builder.CreateRetVoid(), retType, false);
      } else {
        if (retVal->is_ghost_pointer()) {
          if (retType->is_trivially_copyable() || retType->is_trivially_movable()) {
            auto* loadRes = ctx->irCtx->builder.CreateLoad(retType->get_llvm_type(), retVal->get_llvm());
            if (!retType->is_trivially_copyable()) {
              if (!retVal->is_variable()) {
                ctx->Error("This expression does not have variability, and hence cannot be trivially moved from",
                           give_expr.value()->fileRange);
              }
              ctx->irCtx->builder.CreateStore(llvm::Constant::getNullValue(retType->get_llvm_type()),
                                              retVal->get_llvm());
            }
            ir::destructorCaller(ctx->irCtx, fun);
            ir::memberFunctionHandler(ctx->irCtx, fun);
            return ir::Value::get(ctx->irCtx->builder.CreateRet(loadRes), retType, false);
          } else {
            ctx->Error(
                "This expression is of type " + ctx->color(retType->to_string()) +
                    " which is not trivially copyable or movable. To convert the expression to a value, please use " +
                    ctx->color("'copy") + " or " + ctx->color("'move") + " accordingly",
                fileRange);
          }
        } else {
          ir::destructorCaller(ctx->irCtx, fun);
          ir::memberFunctionHandler(ctx->irCtx, fun);
          return ir::Value::get(ctx->irCtx->builder.CreateRet(retVal->get_llvm()), retType, false);
        }
      }
    } else if (retType->is_reference() &&
               retType->as_reference()->get_subtype()->is_same(
                   retVal->is_reference() ? retVal->get_ir_type()->as_reference()->get_subtype()
                                          : retVal->get_ir_type()) &&
               (retType->as_reference()->isSubtypeVariable()
                    ? (retVal->is_ghost_pointer()
                           ? retVal->is_variable()
                           : (retVal->is_reference() && retVal->get_ir_type()->as_reference()->isSubtypeVariable()))
                    : true)) {
      SHOW("Return type is compatible")
      if (retType->is_reference() && !retVal->is_reference() && retVal->is_local_value()) {
        ctx->irCtx->Warning(
            "Returning reference to a local value. The value pointed to by the reference might be destroyed before the function call is complete",
            fileRange);
      }
      if (retVal->is_reference()) {
        retVal->load_ghost_pointer(ctx->irCtx->builder);
      }
      ir::destructorCaller(ctx->irCtx, fun);
      ir::memberFunctionHandler(ctx->irCtx, fun);
      return ir::Value::get(ctx->irCtx->builder.CreateRet(retVal->get_llvm()), retType, false);
    } else if (retVal->get_ir_type()->is_reference() &&
               retVal->get_ir_type()->as_reference()->get_subtype()->is_same(retType)) {
      if (retType->is_trivially_copyable() || retType->is_trivially_movable()) {
        retVal->load_ghost_pointer(ctx->irCtx->builder);
        auto* loadRes = ctx->irCtx->builder.CreateLoad(retType->get_llvm_type(), retVal->get_llvm());
        if (!retType->is_trivially_copyable()) {
          if (!retVal->get_ir_type()->as_reference()->isSubtypeVariable()) {
            ctx->Error("The expression is of type " + ctx->irCtx->color(retVal->get_ir_type()->to_string()) +
                           " which is a reference without variability, and hence cannot be trivially moved from",
                       give_expr.value()->fileRange);
          }
          ctx->irCtx->builder.CreateStore(llvm::Constant::getNullValue(retType->get_llvm_type()), retVal->get_llvm());
        }
        ir::destructorCaller(ctx->irCtx, fun);
        ir::memberFunctionHandler(ctx->irCtx, fun);
        return ir::Value::get(ctx->irCtx->builder.CreateRet(loadRes), retType, false);
      } else {
        ctx->Error(
            "This expression is of type " + ctx->color(retType->to_string()) +
                " which is not trivially copyable or movable. To convert the expression to a value, please use " +
                ctx->color("'copy") + " or " + ctx->color("'move") + " accordingly",
            fileRange);
      }
    } else {
      ctx->Error("Given value type of the function is " +
                     fun->get_ir_type()->as_function()->get_return_type()->to_string() +
                     ", but the provided value in the give sentence is " + retVal->get_ir_type()->to_string(),
                 give_expr.value()->fileRange);
    }
  } else {
    if (fun->get_ir_type()->as_function()->get_return_type()->get_type()->is_void()) {
      ir::destructorCaller(ctx->irCtx, fun);
      ir::memberFunctionHandler(ctx->irCtx, fun);
      return ir::Value::get(ctx->irCtx->builder.CreateRetVoid(), ir::VoidType::get(ctx->irCtx->llctx), false);
    } else {
      ctx->Error("No value is provided to give while the given type of the function is " +
                     ctx->color(fun->get_ir_type()->as_function()->get_return_type()->to_string()) +
                     ". Please provide a value of the appropriate type",
                 fileRange);
    }
  }
}

Json GiveSentence::to_json() const {
  return Json()
      ._("nodeType", "giveSentence")
      ._("hasValue", give_expr.has_value())
      ._("value", give_expr.has_value() ? give_expr.value()->to_json() : Json())
      ._("fileRange", fileRange);
}

} // namespace qat::ast