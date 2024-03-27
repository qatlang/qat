#include "./tuple_value.hpp"
#include "../../IR/logic.hpp"
#include "llvm/IR/DerivedTypes.h"

namespace qat::ast {

ir::Value* TupleValue::emit(EmitCtx* ctx) {
  ir::TupleType* tupleTy = nullptr;
  if (is_type_inferred() || isLocalDecl()) {
    if (isLocalDecl() && !localValue->get_ir_type()->is_tuple()) {
      ctx->Error("Expected expression of type " + ctx->color(localValue->get_ir_type()->to_string()) +
                     ", but found a tuple",
                 fileRange);
    }
    if (!inferredType->is_tuple()) {
      ctx->Error("The inferred type for this tuple expression is " + ctx->color(inferredType->to_string()) +
                     ", which is not a tuple type",
                 fileRange);
    }
    tupleTy = is_type_inferred() ? inferredType->as_tuple() : localValue->get_ir_type()->as_tuple();
    if (inferredType->as_tuple()->getSubTypeCount() != members.size()) {
      ctx->Error("Expected the type of this tuple to be " + ctx->color(inferredType->to_string()) + " with " +
                     ctx->color(std::to_string(inferredType->as_tuple()->getSubTypeCount())) + " members. But " +
                     ctx->color(std::to_string(members.size())) + " values were provided",
                 fileRange);
    }
  }
  Vec<ir::Type*>  tupleMemTys;
  Vec<ir::Value*> tupleMemVals;
  bool            isAllMemsPre = true;
  for (usize i = 0; i < members.size(); i++) {
    auto* expMemTy = tupleTy ? tupleTy->getSubtypeAt(i) : nullptr;
    auto* mem      = members.at(i);
    if (expMemTy) {
      if (mem->has_type_inferrance()) {
        mem->as_type_inferrable()->set_inference_type(expMemTy->is_reference() ? expMemTy->as_reference()->get_subtype()
                                                                               : expMemTy);
      }
    }
    auto* memRes = mem->emit(ctx);
    if (!tupleTy) {
      tupleMemTys.push_back(memRes->get_ir_type()->is_reference() ? memRes->get_ir_type()->as_reference()->get_subtype()
                                                                  : memRes->get_ir_type());
    }
    if (memRes->get_ir_type()->is_reference()) {
      memRes->load_ghost_pointer(ctx->irCtx->builder);
    }
    if (expMemTy) {
      if (expMemTy->is_same(memRes->get_ir_type())) {
        if (memRes->is_ghost_pointer()) {
          auto memType = memRes->get_ir_type();
          if (memType->is_trivially_copyable() || memType->is_trivially_movable()) {
            auto* loadRes = ctx->irCtx->builder.CreateLoad(memType->get_llvm_type(), memRes->get_llvm());
            if (!memType->is_trivially_copyable()) {
              if (!memRes->is_variable()) {
                ctx->Error("This expression does not have variability and hence cannot be trivially moved from",
                           mem->fileRange);
              }
              ctx->irCtx->builder.CreateStore(llvm::Constant::getNullValue(memType->get_llvm_type()),
                                              memRes->get_llvm());
            }
            isAllMemsPre = false;
            tupleMemVals.push_back(ir::Value::get(loadRes, memType, false));
          } else {
            ctx->Error("This expression is of type " + ctx->color(memType->to_string()) +
                           " which is trivially copyable or movable. Please use " + ctx->color("'copy") + " or " +
                           ctx->color("'move") + " accordingly",
                       mem->fileRange);
          }
        } else {
          if (!memRes->is_prerun_value()) {
            isAllMemsPre = false;
          }
          tupleMemVals.push_back(memRes);
        }
      } else if ((expMemTy->is_reference() && expMemTy->as_reference()->is_same(memRes->get_ir_type()) &&
                  (memRes->is_ghost_pointer() &&
                   (expMemTy->as_reference()->isSubtypeVariable() ? memRes->is_variable() : true))) ||
                 (expMemTy->is_reference() && memRes->is_reference() &&
                  expMemTy->as_reference()->get_subtype()->is_same(
                      memRes->get_ir_type()->as_reference()->get_subtype()) &&
                  (expMemTy->as_reference()->isSubtypeVariable()
                       ? memRes->get_ir_type()->as_reference()->isSubtypeVariable()
                       : true))) {
        if (memRes->is_reference()) {
          memRes->load_ghost_pointer(ctx->irCtx->builder);
        }
        isAllMemsPre = false;
        tupleMemVals.push_back(memRes);
      } else if (memRes->is_reference() && memRes->get_ir_type()->as_reference()->get_subtype()->is_same(expMemTy)) {
        memRes->load_ghost_pointer(ctx->irCtx->builder);
        auto memType = memRes->get_ir_type()->as_reference()->get_subtype();
        if (memType->is_trivially_copyable() || memType->is_trivially_movable()) {
          auto* loadRes = ctx->irCtx->builder.CreateLoad(memType->get_llvm_type(), memRes->get_llvm());
          if (!memType->is_trivially_copyable()) {
            if (!memRes->get_ir_type()->as_reference()->isSubtypeVariable()) {
              ctx->Error("This expression is of type " + ctx->color(memRes->get_ir_type()->to_string()) +
                             " which is a reference without variability and hence cannot be trivially moved from",
                         mem->fileRange);
            }
          }
          isAllMemsPre = false;
          tupleMemVals.push_back(ir::Value::get(loadRes, memType, false));
        } else {
          ctx->Error("This expression is a reference of type " + ctx->color(memType->to_string()) +
                         " which is not trivially copyable or movable. Please use " + ctx->color("'copy") + " or " +
                         ctx->color("'move") + " accordingly",
                     mem->fileRange);
        }
      } else {
        ctx->Error("Expected expression of type " + ctx->color(expMemTy->to_string()) +
                       ", but the provided expression is of type " + ctx->color(memRes->get_ir_type()->to_string()),
                   mem->fileRange);
      }
    } else {
      if (!memRes->is_prerun_value()) {
        isAllMemsPre = false;
      }
      tupleMemVals.push_back(memRes);
    }
  }
  if (!tupleTy) {
    tupleTy = ir::TupleType::get(tupleMemTys, isPacked, ctx->irCtx->llctx);
  }
  auto* newLocal = isLocalDecl()
                       ? localValue
                       : ctx->get_fn()->get_block()->new_value(
                             irName.has_value() ? irName.value().value : utils::unique_id(), tupleTy,
                             irName.has_value() ? isVar : true, irName.has_value() ? irName.value().range : fileRange);
  if (isAllMemsPre) {
    Vec<llvm::Constant*> memConst;
    for (auto it : tupleMemVals) {
      memConst.push_back(it->get_llvm_constant());
    }
    ctx->irCtx->builder.CreateStore(
        llvm::ConstantStruct::get(llvm::cast<llvm::StructType>(tupleTy->get_llvm_type()), memConst),
        newLocal->get_llvm());
  } else {
    for (usize i = 0; i < tupleMemVals.size(); i++) {
      ctx->irCtx->builder.CreateStore(
          tupleMemVals.at(i)->get_llvm(),
          ctx->irCtx->builder.CreateStructGEP(tupleTy->get_llvm_type(), newLocal->get_llvm(), i));
    }
  }
  return newLocal->to_new_ir_value();
}

Json TupleValue::to_json() const {
  Vec<JsonValue> mems;
  for (auto mem : members) {
    mems.push_back(mem->to_json());
  }
  return Json()._("nodeType", "tupleValue")._("isPacked", isPacked)._("members", mems)._("fileRange", fileRange);
}

} // namespace qat::ast