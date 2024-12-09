#include "./bitwise_not.hpp"

namespace qat::ast {

ir::PrerunValue* PrerunBitwiseNot::emit(EmitCtx* ctx) {
	auto val   = value->emit(ctx);
	auto valTy = val->get_ir_type();
	if (valTy->is_underlying_type_integer() || valTy->is_underlying_type_unsigned()) {
		return ir::PrerunValue::get(llvm::ConstantExpr::getNot(val->get_llvm_constant()), val->get_ir_type());
	}
	ctx->Error("The value is of type " + ctx->color(val->get_ir_type()->to_string()) +
	               " and hence does not support this operator",
	           fileRange);
	return nullptr;
}

String PrerunBitwiseNot::to_string() const { return "~" + value->to_string(); }

Json PrerunBitwiseNot::to_json() const {
	return Json()._("nodeType", "prerunBitwiseNot")._("value", value->to_json())._("fileRange", fileRange);
}

} // namespace qat::ast