#include "./function_call.hpp"
#include "../../utils/utils.hpp"
#include "llvm/IR/DerivedTypes.h"

namespace qat::ast {

ir::Value* FunctionCall::emit(EmitCtx* ctx) {
  auto* fnVal     = fnExpr->emit(ctx);
  auto  fnValType = fnVal->is_reference() ? fnVal->get_ir_type()->as_reference()->get_subtype() : fnVal->get_ir_type();
  if (fnValType->is_ctype()) {
    fnValType = fnValType->as_ctype()->get_subtype();
  }
  if (fnVal && (fnVal->get_ir_type()->is_function() ||
                (fnValType->is_mark() && fnValType->as_mark()->get_subtype()->is_function()))) {
    auto*                fnTy = fnVal->get_ir_type()->is_function() ? fnVal->get_ir_type()->as_function()
                                                                    : fnValType->as_mark()->get_subtype()->as_function();
    Maybe<ir::Function*> fun;
    if (fnVal->get_ir_type()->is_function()) {
      fun = (ir::Function*)fnVal;
    }
    if (fnTy->get_argument_types().size() != values.size()) {
      ctx->Error("Number of arguments provided for the function call does not "
                 "match the function signature",
                 fileRange);
    }
    Vec<ir::Value*> argsEmit;
    for (usize i = 0; i < values.size(); i++) {
      if (fnTy->get_argument_count() > (u64)i) {
        auto* argTy = fnTy->get_argument_type_at(i)->get_type();
        if (values.at(i)->has_type_inferrance()) {
          values.at(i)->as_type_inferrable()->set_inference_type(argTy);
        }
      }
      argsEmit.push_back(values.at(i)->emit(ctx));
    }
    SHOW("Argument values generated")
    auto fnArgsTy = fnTy->get_argument_types();
    for (usize i = 0; i < fnArgsTy.size(); i++) {
      SHOW("FnArg type is " << fnArgsTy.at(i)->to_string() << " and arg emit type is "
                            << argsEmit.at(i)->get_ir_type()->to_string())
      if (!fnArgsTy.at(i)->get_type()->is_same(argsEmit.at(i)->get_ir_type()) &&
          !fnArgsTy.at(i)->get_type()->isCompatible(argsEmit.at(i)->get_ir_type()) &&
          (argsEmit.at(i)->get_ir_type()->is_reference()
               ? (!fnArgsTy.at(i)->get_type()->is_same(argsEmit.at(i)->get_ir_type()->as_reference()->get_subtype()) &&
                  !fnArgsTy.at(i)->get_type()->isCompatible(
                      argsEmit.at(i)->get_ir_type()->as_reference()->get_subtype()))
               : true)) {
        ctx->Error("Type of this expression is " + ctx->color(argsEmit.at(i)->get_ir_type()->to_string()) +
                       " which is not compatible with the type " + ctx->color(fnArgsTy.at(i)->get_type()->to_string()) +
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
    for (usize i = 0; i < fnArgsTy.size(); i++) {
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
