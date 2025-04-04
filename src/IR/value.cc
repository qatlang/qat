#include "./value.hpp"
#include "../ast/emit_ctx.hpp"
#include "./context.hpp"
#include "./logic.hpp"
#include "./types/function.hpp"
#include "./types/pointer.hpp"
#include "./types/qat_type.hpp"
#include "./types/reference.hpp"

#include <llvm/IR/Instructions.h>

namespace qat::ir {

Value::Value(llvm::Value* _llvmValue, ir::Type* _type, bool _isVariable)
    : type(_type), variable(_isVariable), ll(_llvmValue) {
	allValues.push_back(this);
}

Vec<Value*> Value::allValues = {};

Value* Value::make_local(ast::EmitCtx* ctx, Maybe<String> name, FileRange fileRange) {
	if (not is_ghost_ref()) {
		auto result = ctx->get_fn()->get_block()->new_local(name.value_or(ctx->get_fn()->get_random_alloca_name()),
		                                                    type, true, fileRange);
		ctx->irCtx->builder.CreateStore(get_llvm(), result->get_llvm());
		return result;
	} else {
		return this;
	}
}

ir::Type* Value::get_pass_type() const {
	if (isConfirmedRef || not type->is_ref()) {
		return type;
	} else {
		return type->as_ref()->get_subtype();
	}
}

bool Value::is_prerun_function() const { return is_prerun_value() && get_ir_type()->is_function(); }

Value* Value::call(ir::Ctx* irCtx, const Vec<llvm::Value*>& args, Maybe<u64> _localID,
                   Mod* mod) { // NOLINT(misc-unused-parameters)
	llvm::FunctionType* fnTy  = nullptr;
	ir::FunctionType*   funTy = nullptr;
	if (type->is_ptr() && type->as_ptr()->get_subtype()->is_function()) {
		fnTy  = llvm::dyn_cast<llvm::FunctionType>(type->as_ptr()->get_subtype()->get_llvm_type());
		funTy = type->as_ptr()->get_subtype()->as_function();
	} else {
		fnTy  = llvm::dyn_cast<llvm::FunctionType>(get_ir_type()->get_llvm_type());
		funTy = type->as_function();
	}
	auto result =
	    Value::get(irCtx->builder.CreateCall(fnTy, ll, args),
	               type->is_ptr() ? type->as_ptr()->get_subtype()->as_function()->get_return_type()->get_type()
	                              : type->as_function()->get_return_type()->get_type(),
	               false);
	if (_localID && funTy->get_return_type()->is_return_self()) {
		result->set_local_id(_localID.value());
	}
	return result;
}

void Value::clear_all() {
	for (auto* val : allValues) {
		std::destroy_at(val);
	}
	allValues.clear();
}

bool PrerunValue::is_equal_to(ir::Ctx* irCtx, PrerunValue* other) {
	if (get_ir_type()->is_typed()) {
		if (other->get_ir_type()->is_typed()) {
			return TypeInfo::get_for(get_llvm_constant())
			    ->type->is_same(TypeInfo::get_for(other->get_llvm_constant())->type);
		} else {
			return false;
		}
	} else {
		if (other->get_ir_type()->is_typed()) {
			return false;
		} else {
			return get_ir_type()->equality_of(irCtx, this, other).value_or(false);
		}
	}
}

} // namespace qat::ir
