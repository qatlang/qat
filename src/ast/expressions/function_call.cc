#include "./function_call.hpp"
#include "../../utils/utils.hpp"

#include <llvm/IR/DerivedTypes.h>

namespace qat::ast {

ir::Value* FunctionCall::emit(EmitCtx* ctx) {
  auto* fnVal     = fnExpr->emit(ctx);
  auto  fnValType = fnVal->is_reference() ? fnVal->get_ir_type()->as_reference()->get_subtype() : fnVal->get_ir_type();
  if (fnValType->is_ctype()) {
    fnValType = fnValType->as_ctype()->get_subtype();
  }
  if (fnVal->is_prerun_value() && fnValType->is_function()) {
    // Is prerun function
    auto* fnTy = fnValType->as_function();
    if (fnTy->is_variadic()) {
      if (values.size() < fnTy->get_argument_count()) {
        ctx->Error("The number of arguments provided is " + ctx->color(std::to_string(values.size())) +
                       ", but trying to call a variadic prerun function that requires at least " +
                       ctx->color(std::to_string(fnTy->get_argument_count())) + " arguments",
                   fileRange);
      }
    } else {
      if (values.size() != fnTy->get_argument_count()) {
        ctx->Error("Number of arguments provided for the function call does not match the function signature",
                   fileRange);
      }
    }
    Vec<ir::PrerunValue*> argsEmit;
    for (usize i = 0; i < values.size(); i++) {
      auto argRes = values[i]->emit(ctx);
      if (not argRes->is_prerun_value()) {
        ctx->Error(
            "While trying to perform a prerun function call, but this is not a prerun value. Please check your logic",
            values[i]->fileRange);
      }
      if (i < fnTy->get_argument_count()) {
        if (not fnTy->get_argument_type_at(i)->get_type()->is_same(argRes->get_ir_type())) {
          ctx->Error("Expected a prerun expression of type " +
                         ctx->color(fnTy->get_argument_type_at(i)->get_type()->to_string()) +
                         " here, but found a prerun expression of type " +
                         ctx->color(argRes->get_ir_type()->to_string()),
                     values[i]->fileRange);
        }
      }
      argsEmit.push_back(argRes->as_prerun());
    }
    auto* preFn = (ir::PrerunFunction*)(fnVal->get_llvm_constant());
    return preFn->call_prerun(argsEmit, ctx->irCtx, fileRange);
  } else if (fnVal->get_ir_type()->is_function() ||
             (fnValType->is_mark() && fnValType->as_mark()->get_subtype()->is_function())) {
    auto*                fnTy = fnVal->get_ir_type()->is_function() ? fnVal->get_ir_type()->as_function()
                                                                    : fnValType->as_mark()->get_subtype()->as_function();
    Maybe<ir::Function*> fun;
    if (fnVal->get_ir_type()->is_function()) {
      fun = (ir::Function*)fnVal;
    }
    if (not fnTy->is_variadic() && fnTy->get_argument_count() != values.size()) {
      ctx->Error("Number of arguments provided for the function call does not "
                 "match the function signature",
                 fileRange);
    } else if (fnTy->is_variadic() && (values.size() < fnTy->get_argument_count())) {
      ctx->Error("The number of arguments provided is " + ctx->color(std::to_string(values.size())) +
                     ", but trying to call a variadic function that requires at least " +
                     ctx->color(std::to_string(fnTy->get_argument_count())) + " arguments",
                 fileRange);
    }
    Vec<ir::Value*> argsEmit;
    for (usize i = 0; i < values.size(); i++) {
      if (fnTy->get_argument_count() > (u64)i) {
        auto* argTy = fnTy->get_argument_type_at(i)->get_type();
        if (values[i]->has_type_inferrance()) {
          values[i]->as_type_inferrable()->set_inference_type(argTy);
        }
      }
      argsEmit.push_back(values[i]->emit(ctx));
    }
    SHOW("Argument values generated")
    auto fnArgsTy = fnTy->get_argument_types();
    for (usize i = 0; i < fnTy->get_argument_count(); i++) {
      auto fnArgTy = fnArgsTy[i];
      SHOW("FnArg type is " << fnArgTy->to_string() << " and arg emit type is "
                            << argsEmit.at(i)->get_ir_type()->to_string())
      if ((not fnArgTy->get_type()->is_same(argsEmit.at(i)->get_ir_type())) &&
          (not fnArgTy->get_type()->isCompatible(argsEmit.at(i)->get_ir_type())) &&
          (argsEmit.at(i)->get_ir_type()->is_reference()
               ? ((not fnArgTy->get_type()->is_same(argsEmit.at(i)->get_ir_type()->as_reference()->get_subtype())) &&
                  (not fnArgTy->get_type()->isCompatible(argsEmit.at(i)->get_ir_type()->as_reference()->get_subtype())))
               : true)) {
        ctx->Error("Type of this expression is " + ctx->color(argsEmit.at(i)->get_ir_type()->to_string()) +
                       " which is not compatible with the type " + ctx->color(fnArgTy->get_type()->to_string()) +
                       " of the " + utils::number_to_position(i) + " argument " +
                       (fun.has_value()
                            ? ctx->color(fun.value()->get_ir_type()->as_function()->get_argument_type_at(i)->get_name())
                                  .append(" ")
                            : "") +
                       "of the function " + (fun.has_value() ? ctx->color(fun.value()->get_name().value) : ""),
                   values.at(i)->fileRange);
      }
    }

    Vec<llvm::Value*> argValues;
    for (usize i = 0; i < fnTy->get_argument_count(); i++) {
      SHOW("Argument provided type at " << i << " is: " << argsEmit.at(i)->get_ir_type()->to_string())
      if (fnArgsTy.at(i)->get_type()->is_reference() &&
          (!argsEmit.at(i)->is_reference() && !argsEmit.at(i)->is_ghost_reference())) {
        ctx->Error(
            "Cannot pass a value for the argument that expects a reference. The expression provided does not reside in an address",
            values.at(i)->fileRange);
      } else if (argsEmit.at(i)->is_reference()) {
        argsEmit.at(i)->load_ghost_reference(ctx->irCtx->builder);
        argsEmit.at(i) = ir::Value::get(
            ctx->irCtx->builder.CreateLoad(
                argsEmit.at(i)->get_ir_type()->as_reference()->get_subtype()->get_llvm_type(),
                argsEmit.at(i)->get_llvm()),
            argsEmit.at(i)->get_ir_type(), argsEmit.at(i)->get_ir_type()->as_reference()->isSubtypeVariable());
      } else {
        argsEmit.at(i)->load_ghost_reference(ctx->irCtx->builder);
      }
      argValues.push_back(argsEmit.at(i)->get_llvm());
    }
    if (fnTy->is_variadic()) {
      for (usize i = fnTy->get_argument_count(); i < argsEmit.size(); i++) {
        // The logic of this loop is identical to that in `ast::MethodCall`
        auto currArg  = argsEmit[i];
        auto argTy    = currArg->get_ir_type();
        auto isRefVar = false;
        if (argTy->is_reference()) {
          isRefVar = argTy->as_reference()->isSubtypeVariable();
          argTy    = argTy->as_reference()->get_subtype();
        } else {
          isRefVar = currArg->is_variable();
        }
        if (currArg->get_ir_type()->is_reference() || currArg->is_ghost_reference()) {
          if (currArg->get_ir_type()->is_reference()) {
            currArg->load_ghost_reference(ctx->irCtx->builder);
          }
          if (argTy->is_trivially_copyable() || argTy->is_trivially_movable()) {
            auto* argLLVMVal = currArg->get_llvm();
            argsEmit[i] = ir::Value::get(ctx->irCtx->builder.CreateLoad(argTy->get_llvm_type(), currArg->get_llvm()),
                                         argTy, false);
            if (not argTy->is_trivially_copyable()) {
              if (not isRefVar) {
                ctx->Error("This expression " +
                               String(currArg->get_ir_type()->is_reference() ? "is a reference without variability"
                                                                             : "does not have variability") +
                               " and hence cannot be trivially moved from",
                           values[i]->fileRange);
              }
              ctx->irCtx->builder.CreateStore(llvm::Constant::getNullValue(argTy->get_llvm_type()), argLLVMVal);
            }
          } else {
            ctx->Error("This expression is of type " + ctx->color(currArg->get_ir_type()->to_string()) +
                           " which is not trivially copyable or movable. It cannot be passed as a variadic argument",
                       values[i]->fileRange);
          }
        }
        argValues.push_back(argsEmit[i]->get_llvm());
      }
    }
    if (fun.has_value()) {
      return fun.value()->call(ctx->irCtx, argValues, None, ctx->mod);
    } else {
      fnVal->load_ghost_reference(ctx->irCtx->builder);
      if (fnVal->is_reference()) {
        ctx->irCtx->builder.CreateLoad(fnValType->get_llvm_type(), fnVal->get_llvm());
      }
      return fnVal->call(ctx->irCtx, argValues, None, ctx->mod);
    }
  } else {
    // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
    ctx->Error("The expression is not callable. It has type " + fnVal->get_ir_type()->to_string(), fnExpr->fileRange);
  }
  return nullptr;
}

Json FunctionCall::to_json() const {
  Vec<JsonValue> args;
  for (auto* arg : values) {
    args.emplace_back(arg->to_json());
  }
  return Json()
      ._("nodeType", "functionCall")
      ._("function", fnExpr->to_json())
      ._("arguments", args)
      ._("fileRange", fileRange);
}

} // namespace qat::ast
