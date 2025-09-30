#include "./is_flag_variant.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/flag.hpp"
#include "../../IR/types/unsigned.hpp"

namespace qat::ast {

ir::Value* IsFlagVariant::emit(EmitCtx* ctx) {
	auto cand = candidate->emit(ctx);
	cand      = ir::Logic::handle_pass_semantics(ctx, cand->get_pass_type(), cand, fileRange);
	if (not cand->get_ir_type()->is_flag()) {
		ctx->Error("Expected an expression of a flag type, but got an expression of type " +
		               ctx->color(cand->get_ir_type()->to_string()) + " instead",
		           candidate->fileRange);
	}
	auto         fTy     = cand->get_ir_type()->as_flag();
	llvm::Value* compVal = nullptr;
	if (kind == FlagVariantKind::NONE) {
		compVal = llvm::ConstantInt::get(fTy->get_llvm_type(), 0u);
	} else if (kind == FlagVariantKind::DEFAULT) {
		if (not fTy->has_default_variants()) {
			ctx->irCtx->Warning("The flag type " + ctx->color(fTy->to_string()) +
			                        " does not have any of its variants marked as default. This means that the " +
			                        ctx->color("::{ default }") + " and the " + ctx->color("::{ none }") +
			                        " variants of this type are the same",
			                    fileRange);
		}
		compVal = fTy->get_prerun_default_value(ctx->irCtx)->get_llvm();
	} else {
		const auto bits = fTy->get_underlying_type()->get_bitwidth();
		String     valStr(bits, '0');
		for (usize i = 0; i < variants.size(); i++) {
			for (usize j = i + 1; j < variants.size(); j++) {
				if (variants[i].value == variants[j].value) {
					ctx->Error("The variant " + ctx->color(variants[j].value) + " is repeating here", variants[j].range,
					           std::make_optional(std::make_pair("The previous occurence is here", variants[i].range)));
				}
			}
			auto ind = fTy->get_index_of(variants[i].value);
			if (not ind.has_value()) {
				ctx->Error("The flag type " + ctx->color(fTy->to_string()) + " does not have a variant named " +
				               ctx->color(variants[i].value),
				           variants[i].range);
			}
			if (valStr[ind.value()] == '1') {
				ctx->Error("The variant " + ctx->color(variants[i].value) +
				               " has an alternate name, which has already been provided",
				           variants[i].range);
			}
			valStr[ind.value()] = '1';
		}
		compVal = llvm::ConstantInt::get(llvm::cast<llvm::IntegerType>(fTy->get_llvm_type()), valStr, 2u);
	}
	return ir::Value::get(ctx->irCtx->builder.CreateICmpEQ(cand->get_llvm(), compVal),
	                      ir::UnsignedType::create_bool(ctx->irCtx), true)
	    ->with_range(fileRange);
}

} // namespace qat::ast
