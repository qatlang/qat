#include "./unsigned_literal.hpp"

namespace qat::ast {

ir::PrerunValue* UnsignedLiteral::emit(EmitCtx* ctx) {
	if (is_type_inferred() && not inferredType->is_unsigned() &&
	    (inferredType->is_native_type() && not inferredType->as_native_type()->get_subtype()->is_unsigned())) {
		ctx->Error("The inferred type of this expression is " + inferredType->to_string() +
		               " which is not an unsigned integer type",
		           fileRange);
	}
	if (bits.has_value() && not ctx->mod->has_unsigned_bitwidth(bits.value().first)) {
		ctx->Error("The custom unsigned integer bitwidth " + ctx->color(std::to_string(bits.value().first)) +
		               " is not brought into the module",
		           bits.value().second);
	}
	String intValue = value;
	if (value.find('_') != String::npos) {
		intValue = "";
		for (auto intCh : value) {
			if (intCh != '_') {
				intValue += intCh;
			}
		}
	}
	// NOLINTBEGIN(readability-magic-numbers)
	return ir::PrerunValue::get(
	    llvm::ConstantInt::get(
	        is_type_inferred() ? llvm::dyn_cast<llvm::IntegerType>(inferredType->get_llvm_type())
	                           : llvm::Type::getIntNTy(ctx->irCtx->llctx, bits.has_value() ? bits.value().first : 32u),
	        intValue, 10u),
	    is_type_inferred() ? inferredType
	                       : ir::UnsignedType::create(bits.has_value() ? bits.value().first : 32u, ctx->irCtx));
	// NOLINTEND(readability-magic-numbers)
}

Json UnsignedLiteral::to_json() const {
	return Json()._("nodeType", "unsignedLiteral")._("value", value)._("fileRange", fileRange);
}

String UnsignedLiteral::to_string() const { return value; }

} // namespace qat::ast
