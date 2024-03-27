#include "./constructor_call.hpp"
#include "../../IR/control_flow.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/region.hpp"
#include "llvm/IR/Constants.h"

namespace qat::ast {

ir::Value* ConstructorCall::emit(EmitCtx* ctx) {
  SHOW("Constructor Call - Emitting type")
  if (!type.has_value() && !is_type_inferred()) {
    ctx->Error("No type provided for constructor call, and no type inferred from scope", fileRange);
  }
  auto* typ = type.has_value() ? type.value()->emit(ctx) : inferredType;
  if (type.has_value() && is_type_inferred()) {
    if (!typ->is_same(inferredType)) {
      ctx->Error("The provided type for the constructor call is " + ctx->color(typ->to_string()) +
                     " but the inferred type is " + ctx->color(inferredType->to_string()),
                 fileRange);
    }
  }
  SHOW("Emitted type")
  if (typ->is_expanded()) {
    auto*                             eTy = typ->as_expanded();
    Vec<ir::Value*>                   valsIR;
    Vec<Pair<Maybe<bool>, ir::Type*>> valsType;
    for (auto* arg : args) {
      auto* argVal = arg->emit(ctx);
      valsType.push_back(
          {argVal->is_ghost_pointer() ? Maybe<bool>(argVal->is_variable()) : None, argVal->get_ir_type()});
      valsIR.push_back(argVal);
    }
    auto reqInfo = ctx->get_access_info();
    SHOW("Argument values emitted for function call")
    ir::Method* cons = nullptr;
    if (args.empty()) {
      if (eTy->has_default_constructor()) {
        cons = eTy->get_default_constructor();
        if (!cons->is_accessible(reqInfo)) {
          ctx->Error("The default constructor of type " + ctx->color(eTy->get_full_name()) + " is not accessible here",
                     fileRange);
        }
        SHOW("Found default constructor")
      } else {
        ctx->Error(
            "Type " + ctx->color(eTy->to_string()) +
                " does not have a default constructor and hence arguments have to be provided in this constructor call",
            fileRange);
      }
    } else if (args.size() == 1) {
      if (eTy->has_from_convertor(valsType[0].first, valsType[0].second)) {
        cons = eTy->get_from_convertor(valsType[0].first, valsType[0].second);
        if (!cons->is_accessible(reqInfo)) {
          ctx->Error("This convertor of type " + ctx->color(eTy->get_full_name()) + " is not accessible here",
                     fileRange);
        }
        SHOW("Found convertor with type")
      } else {
        ctx->Error("No from convertor found for type " + ctx->color(eTy->get_full_name()) + " with type " +
                       ctx->color(valsType.front().second->to_string()),
                   fileRange);
      }
    } else {
      if (eTy->has_constructor_with_types(valsType)) {
        cons = eTy->get_constructor_with_types(valsType);
        if (!cons->is_accessible(reqInfo)) {
          ctx->Error("This constructor of type " + ctx->color(eTy->get_full_name()) + " is not accessible here",
                     fileRange);
        }
        SHOW("Found constructor with types")
      } else {
        ctx->Error("No matching constructor found for type " + ctx->color(eTy->get_full_name()), fileRange);
      }
    }
    SHOW("Found convertor/constructor")
    // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
    auto argTys = cons->get_ir_type()->as_function()->get_argument_types();
    for (usize i = 1; i < argTys.size(); i++) {
      if (argTys.at(i)->get_type()->is_reference()) {
        if (!valsType.at(i - 1).second->is_reference()) {
          if (!valsType.at(i - 1).first.has_value()) {
            valsIR[i = 1] = valsIR[i - 1]->make_local(ctx, None, args[i - 1]->fileRange);
          }
          if (argTys.at(i)->get_type()->as_reference()->isSubtypeVariable() && !valsIR.at(i - 1)->is_variable()) {
            ctx->Error("The expected argument type is " + ctx->color(argTys.at(i)->get_type()->to_string()) +
                           " but the provided value is not a variable",
                       args.at(i - 1)->fileRange);
          }
        } else {
          if (argTys.at(i)->get_type()->as_reference()->isSubtypeVariable() &&
              !valsIR.at(i - 1)->get_ir_type()->as_reference()->isSubtypeVariable()) {
            ctx->Error("The expected argument type is " + ctx->color(argTys.at(i)->get_type()->to_string()) +
                           " but the provided value is of type " +
                           ctx->color(valsIR.at(i - 1)->get_ir_type()->to_string()),
                       args.at(i - 1)->fileRange);
          }
        }
      } else {
        auto* valTy = valsType.at(i - 1).second;
        auto* val   = valsIR.at(i - 1);
        if (valTy->is_reference() || val->is_ghost_pointer()) {
          if (valTy->is_reference()) {
            valTy = valTy->as_reference()->get_subtype();
          }
          valsIR.at(i - 1) =
              ir::Value::get(ctx->irCtx->builder.CreateLoad(valTy->get_llvm_type(), val->get_llvm()), valTy, false);
        }
      }
    }
    SHOW("About to create llAlloca")
    llvm::Value* llAlloca;
    if (isLocalDecl()) {
      llAlloca = localValue->getAlloca();
    } else {
      auto newAlloca = ctx->get_fn()->get_block()->new_value(
          irName.has_value() ? irName.value().value : ctx->get_fn()->get_random_alloca_name(), eTy, isVar,
          irName.has_value() ? irName.value().range : fileRange);
      llAlloca = newAlloca->get_llvm();
    }
    Vec<llvm::Value*> valsLLVM;
    valsLLVM.push_back(llAlloca);
    for (auto* val : valsIR) {
      valsLLVM.push_back(val->get_llvm());
    }
    (void)cons->call(ctx->irCtx, valsLLVM, None, ctx->mod);
    if (isLocalDecl()) {
      return localValue->to_new_ir_value();
    } else {
      auto* res = ir::Value::get(llAlloca, eTy, irName.has_value() ? isVar : true);
      if (isLocalDecl()) {
        res->set_local_id(localValue->get_local_id().value());
      }
      return res;
    }
  } else {
    ctx->Error("The provided type " + ctx->color(typ->to_string()) + " cannot be constructed",
               type.has_value() ? type.value()->fileRange : fileRange);
  }
  return nullptr;
}

Json ConstructorCall::to_json() const {
  Vec<JsonValue> argsJson;
  for (auto* arg : args) {
    argsJson.push_back(arg->to_json());
  }
  return Json()
      ._("nodeType", "constructorCall")
      ._("hasType", type.has_value())
      ._("type", type.has_value() ? type.value()->to_json() : JsonValue())
      ._("arguments", argsJson)
      ._("fileRange", fileRange);
}

} // namespace qat::ast