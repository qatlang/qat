#include "./error.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/result.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

void ErrorExpression::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent,
                                          EmitCtx* ctx) {
  if (errorValue) {
    UPDATE_DEPS(errorValue);
  }
  if (isPacked.has_value() && isPacked.value().second) {
    UPDATE_DEPS(isPacked.value().second);
  }
  if (providedType.has_value()) {
    UPDATE_DEPS(providedType.value().first);
    UPDATE_DEPS(providedType.value().second);
  }
}

ir::Value* ErrorExpression::emit(EmitCtx* ctx) {
  if (!ctx->has_fn()) {
    ctx->Error("Expected this expression to be inside a function", fileRange);
  }
  FnAtEnd endFn      = FnAtEnd([&]() { createIn = nullptr; });
  auto    usableType = inferredType;
  bool    packValue  = false;
  if (isPacked.has_value()) {
    if (isPacked.value().second) {
      auto packExp = isPacked.value().second->emit(ctx);
      if (!packExp->get_ir_type()->is_bool()) {
        ctx->Error("The condition for packing the " + ctx->color("result") + " datatype is expected to be of " +
                       ctx->color("bool") + " type. Got an expression of type " +
                       ctx->color(packExp->get_ir_type()->to_string()),
                   isPacked.value().second->fileRange);
      }
      packValue = llvm::cast<llvm::ConstantInt>(packExp->get_llvm_constant())->getValue().getBoolValue();
    } else {
      packValue = true;
    }
  }
  if (providedType.has_value()) {
    auto provResTy = providedType.value().first->emit(ctx);
    auto provErrTy = providedType.value().second->emit(ctx);
    auto provTy    = ir::ResultType::get(provResTy, provErrTy, packValue, ctx->irCtx);
    check_inferred_type(provTy, ctx, fileRange);
    usableType = provTy;
  }
  if (usableType) {
    if (usableType->is_result()) {
      auto* resTy = usableType->as_result();
      if (isPacked.has_value() && (resTy->isTypePacked() != packValue)) {
        ctx->Error(ctx->color(resTy->to_string()) + (resTy->isTypePacked() ? " is" : " is not") +
                       " a packed datatype, which does not match with the provided condition, which evaluates to " +
                       ctx->color(packValue ? "true" : "false"),
                   isPacked.value().first);
      }
      auto* errTy = resTy->getErrorType();
      if (!errorValue) {
        if (!errTy->is_void()) {
          ctx->Error("The inferred " + ctx->color("result") + " type for this expression is " +
                         ctx->color(resTy->to_string()) +
                         ", so expected an expression that is compatible with the error type " +
                         ctx->color(errTy->to_string()) + " to be provided",
                     fileRange);
        }
      } else if (errorValue->has_type_inferrance()) {
        errorValue->as_type_inferrable()->set_inference_type(errTy);
      }
      llvm::Value* newAlloc = nullptr;
      if (isLocalDecl()) {
        newAlloc = localValue->get_llvm();
      } else if (canCreateIn()) {
        if (createIn->is_reference() || createIn->is_ghost_reference()) {
          auto expTy = createIn->is_ghost_reference() ? createIn->get_ir_type()
                                                      : createIn->get_ir_type()->as_reference()->get_subtype();
          if (!expTy->is_same(usableType)) {
            ctx->Error("Tried to optimise the " + ctx->color("error") +
                           " expression by creating in-place, but the inferred type is " +
                           ctx->color(usableType->to_string()) +
                           " which does not match with the underlying type for in-place creation which is " +
                           ctx->color(expTy->to_string()),
                       fileRange);
          }
        } else {
          ctx->Error("Tried to optimise the " + ctx->color("error") +
                         " expression by creating in-place, but the underlying type for in-place creation is " +
                         ctx->color(createIn->get_ir_type()->to_string()) + ", which is not a reference",
                     fileRange);
        }
      } else if (irName.has_value()) {
        newAlloc =
            ctx->get_fn()->get_block()->new_value(irName.value().value, resTy, isVar, irName.value().range)->get_llvm();
      } else {
        newAlloc = ir::Logic::newAlloca(ctx->get_fn(), None, resTy->get_llvm_type());
      }
      const auto shouldCreateIn = !errTy->is_void() && errorValue && errorValue->isInPlaceCreatable();
      if (shouldCreateIn) {
        // TODO - Check if the reference type below can have variation. It should be in almost all cases,
        // since the reference is supposed to facilitate in-place creation
        errorValue->asInPlaceCreatable()->setCreateIn(
            ir::Value::get(ctx->irCtx->builder.CreatePointerCast(
                               ctx->irCtx->builder.CreateStructGEP(resTy->get_llvm_type(), newAlloc, 1u),
                               errTy->get_llvm_type()->getPointerTo(0u)),
                           ir::ReferenceType::get(true, errTy, ctx->irCtx), false));
      }
      auto* errVal = errorValue ? errorValue->emit(ctx) : nullptr;
      if (errTy->is_void()) {
        if (errVal && !errVal->get_ir_type()->is_void()) {
          ctx->Error("Type of this expression is " + ctx->color(errVal->get_ir_type()->to_string()) +
                         " which does not match with the error type of " + ctx->color(resTy->to_string()) +
                         ", which is " + ctx->color(errTy->to_string()),
                     errorValue->fileRange);
        }
        resTy->handle_tag_store(newAlloc, false, ctx->irCtx);
        return ir::Value::get(ctx->irCtx->builder.CreateLoad(resTy->get_llvm_type(), newAlloc), resTy, true);
      }
      if (shouldCreateIn) {
        // NOTE - Condition means that the sub-expression has already been created in-place
        resTy->handle_tag_store(newAlloc, false, ctx->irCtx);
        errorValue->asInPlaceCreatable()->unsetCreateIn();
        return createIn;
      }
      // NOTE - The following function checks for the type of the expression
      auto* finalErr = ir::Logic::handle_pass_semantics(ctx, errTy, errVal, errorValue->fileRange);
      resTy->handle_tag_store(newAlloc, false, ctx->irCtx);
      ctx->irCtx->builder.CreateStore(finalErr->get_llvm(),
                                      ctx->irCtx->builder.CreatePointerCast(
                                          ctx->irCtx->builder.CreateStructGEP(resTy->get_llvm_type(), newAlloc, 1u),
                                          errTy->get_llvm_type()->getPointerTo(0u)));
      return ir::Value::get(ctx->irCtx->builder.CreateLoad(resTy->get_llvm_type(), newAlloc), resTy, false);
    } else {
      ctx->Error("The inferred type is " + ctx->color(usableType->to_string()) + " but creating an error requires a " +
                     ctx->color("result") + " type",
                 fileRange);
    }
  } else {
    ctx->Error("No inferred type found for this expression, and no type were provided. "
               "You can provide a type for error expression like " +
                   ctx->color("error:[validType, errorTy](expression)"),
               fileRange);
  }
  return nullptr;
}

Json ErrorExpression::to_json() const {
  return Json()._("nodeType", "errorExpression")._("errorValue", errorValue->to_json())._("fileRange", fileRange);
}

} // namespace qat::ast
