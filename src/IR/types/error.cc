#include "./error.hpp"
#include "../context.hpp"
#include "../value.hpp"

namespace qat::ir {

Vec<ErrorType*> ErrorType::allErrorTypes = {};

ir::PrerunValue* ErrorType::get_prerun_default_value(ir::Ctx*) {
	return ir::PrerunValue::get(llvm::Constant::getNullValue(llvmType), this);
}

Maybe<String> ErrorType::to_prerun_generic_string(ir::PrerunValue* val) const {
	if (has_simple_move() && val->get_llvm_constant()->isNullValue()) {
		return "error::none";
	} else {
		auto res = subType->to_prerun_generic_string(ir::PrerunValue::get(val->get_llvm_constant(), subType));
		if (res.has_value()) {
			return "error(" + res.value() + ")";
		}
		return None;
	}
}

void ErrorType::default_construct_value(ir::Ctx* irCtx, ir::Value* instance, ir::Function* fun) {
	if (not is_default_constructible()) {
		irCtx->Error("Could not default construct value of type " + irCtx->color(to_string()), None);
	}
	if (has_simple_move()) {
		irCtx->builder.CreateStore(llvm::Constant::getNullValue(llvmType), instance->get_llvm());
	} else {
		subType->default_construct_value(irCtx, instance, fun);
	}
}

Maybe<bool> ErrorType::equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const {
	return subType->equality_of(irCtx, ir::PrerunValue::get(first->get_llvm_constant(), subType),
	                            ir::PrerunValue::get(second->get_llvm_constant(), subType));
}

} // namespace qat::ir
