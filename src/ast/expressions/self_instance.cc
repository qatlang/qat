#include "./self_instance.hpp"

namespace qat::ast {

ir::Value* SelfInstance::emit(EmitCtx* ctx) {
	if (ctx->get_fn()->is_method()) {
		auto* mFn = (ir::Method*)ctx->get_fn();
		if (mFn->get_method_type() == ir::MethodType::staticFn) {
			ctx->Error("This is a static method and hence the parent instance cannot be retrieved", fileRange);
		}
		if (mFn->get_first_block()->has_value("''")) {
			auto selfVal = mFn->get_first_block()->get_value("''");
			return ir::Value::get(selfVal->get_llvm(), selfVal->get_ir_type(),
			                      mFn->get_method_type() == ir::MethodType::valueMethod);
		} else {
			if (mFn->get_ir_type()->as_function()->get_argument_type_at(0)->get_name() == "''") {
				return ir::Value::get(mFn->get_llvm_function()->getArg(0),
				                      mFn->get_ir_type()->as_function()->get_argument_type_at(0)->get_type(),
				                      mFn->get_method_type() == ir::MethodType::valueMethod);
			} else {
				ctx->Error("Cannot retrieve the parent instance of this method", fileRange);
			}
		}
	} else {
		ctx->Error("The current function is not a method of any type and hence the parent instance cannot be retrieved",
		           fileRange);
	}
}

Json SelfInstance::to_json() const { return Json()._("nodeType", "selfInstance")._("fileRange", fileRange); }

} // namespace qat::ast
