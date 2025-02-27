#include "./flag_initialiser.hpp"
#include "../../IR/types/flag.hpp"
#include "../../IR/types/unsigned.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

ir::PrerunValue* FlagInitialiser::emit(EmitCtx* ctx) {
	ir::Type* useType = type ? type->emit(ctx) : (inferredType ? inferredType : nullptr);
	if (useType == nullptr) {
		ctx->Error("The flag type for this expression was not provided, and no type could be inferred from scope",
		           fileRange);
	}
	if (type && inferredType && (not useType->is_same(inferredType))) {
		ctx->Error("The flag type provided is " + ctx->color(useType->to_string()) +
		               ", but the type inferred from scope is " + ctx->color(inferredType->to_string()),
		           fileRange);
	}
	if (not useType->is_flag()) {
		ctx->Error("The type of this expression is " + ctx->color(useType->to_string()) + ", which is not a flag type",
		           fileRange);
	}
	if (specialRange.has_value()) {
		if (isSpecialDefault) {
			return useType->as_flag()->get_prerun_default_value(ctx->irCtx);
		} else {
			return ir::PrerunValue::get(llvm::ConstantInt::get(useType->get_llvm_type(), 0u, false), useType);
		}
	} else {
		std::map<usize, Identifier> foundVariants;
		for (usize i = 0; i < variants.size(); i++) {
			auto ind = useType->as_flag()->get_index_of(variants[i].value);
			if (not ind.has_value()) {
				ctx->Error("The flag type " + ctx->color(useType->to_string()) + " does not have a variant named " +
				               ctx->color(variants[i].value) + ". Please check the logic",
				           variants[i].range);
			}
			if (foundVariants.contains(ind.value())) {
				ctx->irCtx->Warning("The variant " + ctx->color(variants[i].value) + " of the flag type " +
				                        ctx->color(useType->to_string()) + " has the same value as the variant " +
				                        ctx->color(foundVariants.at(ind.value()).value) +
				                        ", so repeating it here is unnecessary",
				                    variants[i].range);
			} else {
				foundVariants.insert(std::make_pair(ind.value(), variants[i]));
			}
		}
		auto   underTy = useType->as_flag()->get_underlying_type();
		String valStr  = "";
		for (usize i = 0; i < underTy->get_bitwidth(); i++) {
			valStr += foundVariants.contains(i) ? "1" : "0";
		}
		return ir::PrerunValue::get(
		    llvm::ConstantInt::get(llvm::cast<llvm::IntegerType>(underTy->get_llvm_type()), valStr, 2u), useType);
	}
}

} // namespace qat::ast
