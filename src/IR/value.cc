#include "./value.hpp"
#include "../ast/emit_ctx.hpp"
#include "./types/qat_type.hpp"
#include "context.hpp"
#include "logic.hpp"
#include "types/function.hpp"
#include "types/pointer.hpp"
#include "llvm/IR/Instructions.h"

namespace qat::ir {

Value::Value(llvm::Value* _llvmValue, ir::Type* _type, bool _isVariable)
    : type(_type), variable(_isVariable), ll(_llvmValue) {
  allValues.push_back(this);
}

Vec<Value*> Value::allValues = {};

Value* Value::make_local(ast::EmitCtx* ctx, Maybe<String> name, FileRange fileRange) {
  if (!is_ghost_pointer()) {
    auto result = ctx->get_fn()->get_block()->new_value(name.value_or(ctx->get_fn()->get_random_alloca_name()), type,
                                                        true, fileRange);
    ctx->irCtx->builder.CreateStore(get_llvm(), result->get_llvm());
    return result;
  } else {
    return this;
  }
}

Value* Value::call(ir::Ctx* irCtx, const Vec<llvm::Value*>& args, Maybe<String> _localID,
                   Mod* mod) { // NOLINT(misc-unused-parameters)
  llvm::FunctionType* fnTy  = nullptr;
  ir::FunctionType*   funTy = nullptr;
  if (type->is_pointer() && type->as_pointer()->get_subtype()->is_function()) {
    fnTy  = llvm::dyn_cast<llvm::FunctionType>(type->as_pointer()->get_subtype()->get_llvm_type());
    funTy = type->as_pointer()->get_subtype()->as_function();
  } else {
    fnTy  = llvm::dyn_cast<llvm::FunctionType>(get_ir_type()->get_llvm_type());
    funTy = type->as_function();
  }
  auto result =
      new Value(irCtx->builder.CreateCall(fnTy, ll, args),
                type->is_pointer() ? type->as_pointer()->get_subtype()->as_function()->get_return_type()->get_type()
                                   : type->as_function()->get_return_type()->get_type(),
                false);
  if (_localID && funTy->get_return_type()->is_return_self()) {
    result->set_local_id(_localID.value());
  }
  return result;
}

void Value::clear_all() {
  for (auto* val : allValues) {
    delete val;
  }
  allValues.clear();
}

PrerunValue::PrerunValue(llvm::Constant* _llConst, ir::Type* _type) : Value(_llConst, _type, false) {}

PrerunValue::PrerunValue(ir::TypedType* _typed) : Value(nullptr, _typed, false) {}

llvm::Constant* PrerunValue::get_llvm() const { return (llvm::Constant*)ll; }

bool PrerunValue::is_prerun_value() const { return true; }

bool PrerunValue::is_equal_to(ir::Ctx* irCtx, PrerunValue* other) {
  if (get_ir_type()->is_typed()) {
    if (other->get_ir_type()->is_typed()) {
      return get_ir_type()->as_typed()->get_subtype()->is_same(other->get_ir_type()->as_typed()->get_subtype());
    } else {
      return false;
    }
  } else {
    if (as_prerun()->get_ir_type()->is_typed()) {
      if (other->get_ir_type()->is_typed()) {
        return as_prerun()->get_ir_type()->as_typed()->get_subtype()->is_same(
            other->get_ir_type()->as_typed()->get_subtype());
      } else {
        return false;
      }
    } else if (other->get_ir_type()->is_typed()) {
      return false;
    } else {
      return get_ir_type()->equality_of(irCtx, this, other).value_or(false);
    }
  }
}

} // namespace qat::ir