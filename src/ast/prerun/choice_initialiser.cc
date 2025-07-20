#include "./choice_initialiser.hpp"

namespace qat::ast {

ir::PrerunValue* PrerunChoiceInitialiser::emit(EmitCtx* ctx) {
	auto typeEmit = type ? type.emit(ctx) : inferredType;
	if (type && is_type_inferred() && not typeEmit->is_same(inferredType)) {
		ctx->Error("The type inferred from scope for this expression is " + ctx->color(inferredType->to_string()) +
		               ", but the provided type is " + ctx->color(typeEmit->to_string()) + ". These do not match",
		           fileRange);
	}
	if (not typeEmit->is_choice()) {
		ctx->Error("This expression expects a choice type, but the " +
		               String(type ? "type provided" : "inferred type") + " is " + ctx->color(typeEmit->to_string()) +
		               ", which is not a choice type",
		           fileRange);
	}
	auto* chTy = typeEmit->as_choice();
	if (not chTy->has_field(variant.value)) {
		ctx->Error("Choice type " + ctx->color(chTy->to_string()) + " does not have a variant named " +
		               ctx->color(variant.value),
		           variant.range);
	}
	return ir::PrerunValue::get(chTy->get_value_for(variant.value), chTy);
}

} // namespace qat::ast
