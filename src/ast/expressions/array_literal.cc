#include "./array_literal.hpp"
#include "llvm/IR/Constants.h"

namespace qat::ast {

ir::Value* ArrayLiteral::emit(EmitCtx* ctx) {
  FnAtEnd        fnObj{[&] { createIn = nullptr; }};
  ir::ArrayType* arrTyOfLocal = nullptr;
  if (isLocalDecl()) {
    if (localValue->get_ir_type()->is_array()) {
      arrTyOfLocal = localValue->get_ir_type()->as_array();
    } else {
      ctx->Error("The type for the local declaration is " + ctx->color(localValue->get_ir_type()->to_string()) +
                     ". This expression expects to be of array type",
                 fileRange);
    }
  }
  if (isLocalDecl() && (arrTyOfLocal->get_length() != values.size())) {
    ctx->Error("The expected length of the array is " + ctx->color(std::to_string(arrTyOfLocal->get_length())) +
                   ", but this array literal has " + ctx->color(std::to_string(values.size())) + " elements",
               fileRange);
  } else if (isTypeInferred() && inferredType->is_array() && inferredType->as_array()->get_length() != values.size()) {
    auto infLen = inferredType->as_array()->get_length();
    ctx->Error("The length of the array type inferred is " + ctx->color(std::to_string(infLen)) + ", but " +
                   ((values.size() < infLen) ? "only " : "") + ctx->color(std::to_string(values.size())) + " value" +
                   ((values.size() > 1u) ? "s were" : "was") + " provided.",
               fileRange);
  } else if (isTypeInferred() && !inferredType->is_array()) {
    ctx->Error("Type inferred from scope for this expression is " + ctx->color(inferredType->to_string()) +
                   ". This expression expects to be of array type",
               fileRange);
  }
  Vec<ir::Value*> valsIR;
  auto*           elemInferredTy     = arrTyOfLocal ? arrTyOfLocal->get_element_type()
                                                    : (inferredType ? inferredType->as_array()->get_element_type() : nullptr);
  bool            areAllValsConstant = true;
  for (auto* val : values) {
    if ((arrTyOfLocal || isTypeInferred()) && val->hasTypeInferrance() && elemInferredTy) {
      val->asTypeInferrable()->setInferenceType(elemInferredTy);
    }
    valsIR.push_back(val->emit(ctx));
    if (!valsIR.back()->is_prerun_value()) {
      areAllValsConstant = false;
    }
  }
  SHOW("Getting element type")
  ir::Type* elemTy = nullptr;
  if (!values.empty()) {
    if (valsIR.front()->is_reference()) {
      elemTy = valsIR.front()->get_ir_type()->as_reference()->get_subtype();
    } else {
      elemTy = valsIR.front()->get_ir_type();
    }
    for (usize i = 1; i < valsIR.size(); i++) {
      auto* val = valsIR.at(i);
      if (val->get_ir_type()->is_same(elemTy) ||
          (val->get_ir_type()->is_reference() && val->get_ir_type()->as_reference()->get_subtype()->is_same(elemTy))) {
        val->load_ghost_pointer(ctx->irCtx->builder);
        if (val->get_ir_type()->is_reference()) {
          valsIR.at(i) =
              ir::Value::get(ctx->irCtx->builder.CreateLoad(
                                 val->get_ir_type()->as_reference()->get_subtype()->get_llvm_type(), val->get_llvm()),
                             val->get_ir_type()->as_reference()->get_subtype(),
                             val->get_ir_type()->as_reference()->isSubtypeVariable());
        }
      } else {
        ctx->Error("The expected type is " + ctx->color(elemTy->to_string()) +
                       " but the provided expression is of type " + ctx->color(val->get_ir_type()->to_string()) +
                       ". These do not match",
                   values.at(i)->fileRange);
      }
    }
    // TODO - Implement constant array literals
    llvm::Value*    alloca   = nullptr;
    llvm::Constant* constVal = nullptr;
    if (areAllValsConstant && !isLocalDecl() && !irName.has_value()) {
      Vec<llvm::Constant*> valsConst;
      for (auto v : valsIR) {
        valsConst.push_back(v->get_llvm_constant());
      }
      constVal = llvm::ConstantArray::get(llvm::ArrayType::get(elemTy->get_llvm_type(), valsIR.size()), valsConst);
      if (!isLocalDecl() && !irName.has_value()) {
        return ir::PrerunValue::get(constVal, ir::ArrayType::get(elemTy, valsIR.size(), ctx->irCtx->llctx));
      }
    }
    if (isLocalDecl()) {
      alloca = localValue->get_llvm();
    } else if (canCreateIn()) {
      alloca = createIn->get_llvm();
    } else {
      auto* loc = ctx->get_fn()->get_block()->new_value(irName.has_value() ? irName->value
                                                                           : ctx->get_fn()->get_random_alloca_name(),
                                                        ir::ArrayType::get(elemTy, values.size(), ctx->irCtx->llctx),
                                                        isVar, irName.has_value() ? irName->range : fileRange);
      alloca = loc->getAlloca();
    }
    if (constVal) {
      ctx->irCtx->builder.CreateStore(constVal, alloca);
      return ir::Value::get(alloca, ir::ArrayType::get(elemTy, valsIR.size(), ctx->irCtx->llctx), false);
    }
    auto* elemPtr = ctx->irCtx->builder.CreateInBoundsGEP(
        ir::ArrayType::get(elemTy, valsIR.size(), ctx->irCtx->llctx)->get_llvm_type(), alloca,
        {llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->irCtx->llctx), 0u),
         llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->irCtx->llctx), 0u)});
    ctx->irCtx->builder.CreateStore(valsIR.at(0)->get_llvm(), elemPtr);
    for (usize i = 1; i < valsIR.size(); i++) {
      elemPtr = ctx->irCtx->builder.CreateInBoundsGEP(
          elemTy->get_llvm_type(), elemPtr, llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->irCtx->llctx), 1u));
      SHOW("Value at " << i << " is of type " << valsIR.at(i)->getType()->to_string())
      ctx->irCtx->builder.CreateStore(valsIR.at(i)->get_llvm(), elemPtr);
    }
    return ir::Value::get(alloca, ir::ArrayType::get(elemTy, valsIR.size(), ctx->irCtx->llctx),
                          isLocalDecl() ? localValue->is_variable() : isVar);
  } else {
    if (isLocalDecl()) {
      if (localValue->get_ir_type()->is_array()) {
        ctx->irCtx->builder.CreateStore(
            llvm::ConstantArray::get(llvm::cast<llvm::ArrayType>(arrTyOfLocal->get_llvm_type()), {}),
            localValue->get_llvm());
      }
    } else {
      if (inferredType->as_array()->get_element_type()) {
        return ir::PrerunValue::get(
            llvm::ConstantArray::get(
                llvm::ArrayType::get(inferredType->as_array()->get_element_type()->get_llvm_type(), 0u), {}),
            ir::ArrayType::get(inferredType->as_array()->get_element_type(), 0u, ctx->irCtx->llctx));
      } else {
        ctx->Error("Element type for the empty array is not provided and could not be inferred", fileRange);
      }
    }
    return nullptr;
  }
}

Json ArrayLiteral::to_json() const {
  Vec<JsonValue> vals;
  for (auto* exp : values) {
    vals.push_back(exp->to_json());
  }
  return Json()._("nodeType", "arrayLiteral")._("values", vals);
}

} // namespace qat::ast