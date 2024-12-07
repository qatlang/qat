#include "./generic_integer.hpp"
#include "../expression.hpp"

namespace qat::ast {

void GenericIntegerType::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* entity,
											 EmitCtx* ctx) {
	bitValue->update_dependencies(phase, ir::DependType::complete, entity, ctx);
	if (isUnsignedExp) {
		isUnsignedExp->update_dependencies(phase, ir::DependType::complete, entity, ctx);
	}
}

ir::Type* GenericIntegerType::emit(EmitCtx* ctx) {
	bool isTypeUnsigned = false;
	if (isUnsignedExp) {
		auto res = isUnsignedExp->emit(ctx);
		if (res->get_ir_type()->is_bool()) {
			isTypeUnsigned = llvm::cast<llvm::ConstantInt>(res->get_llvm())->getValue().getBoolValue();
		} else {
			ctx->Error("Expected this expression to be of type " + ctx->color("bool") +
						   " as it is used to determine whether this integer type should be signed or unsigned",
					   isUnsignedExp->fileRange);
		}
	} else {
		isTypeUnsigned = isUnsigned.value();
	}
	if (bitValue->has_type_inferrance()) {
		bitValue->as_type_inferrable()->set_inference_type(ir::UnsignedType::create(32, ctx->irCtx));
	}
	auto bitsExp = bitValue->emit(ctx);
	if (!bitsExp->get_ir_type()->is_unsigned_integer() ||
		!bitsExp->get_ir_type()->as_unsigned_integer()->is_bitWidth(32)) {
		ctx->Error("This expression is expected to be of " + ctx->color("u32") + " type", bitValue->fileRange);
	}
	auto typeBits = *llvm::cast<llvm::ConstantInt>(bitsExp->get_llvm())->getValue().getRawData();
	if (isTypeUnsigned) {
		return ir::UnsignedType::create(typeBits, ctx->irCtx);
	} else {
		return ir::IntegerType::get(typeBits, ctx->irCtx);
	}
}

Json GenericIntegerType::to_json() const {
	return Json()
		._("typeKind", "genericInteger")
		._("bits", bitValue->to_json())
		._("hasUnsignedValue", isUnsigned.has_value())
		._("isUnsignedValue", isUnsigned.has_value() ? isUnsigned.value() : JsonValue())
		._("hasUnsignedExpr", isUnsignedExp != nullptr)
		._("isUnsignedExpr", isUnsignedExp ? isUnsignedExp->to_json() : Json())
		._("fileRange", fileRange);
}

String GenericIntegerType::to_string() const {
	return isUnsignedExp ? ("integer:[" + isUnsignedExp->to_string() + ", " + bitValue->to_string() + "]")
						 : ((isUnsigned ? "uint:[" : "int:[") + bitValue->to_string() + "]");
}

} // namespace qat::ast
