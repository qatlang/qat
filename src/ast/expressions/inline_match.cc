#include "./inline_match.hpp"
#include "../../IR/control_flow.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/maybe.hpp"
#include "../../IR/types/native_type.hpp"
#include "../../IR/types/pointer.hpp"
#include "../../IR/types/result.hpp"

namespace qat::ast {

void InlineMatch::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) {
	for (auto val : values) {
		UPDATE_DEPS(val);
	}
}

ir::Value* InlineMatch::emit(EmitCtx* ctx) {
	auto       expr  = expression->emit(ctx);
	const bool isRef = expr->get_ir_type()->is_ref();
	auto valTy = expr->get_ir_type()->is_ref() ? expr->get_ir_type()->as_ref()->get_subtype() : expr->get_ir_type();
	Vec<Pair<llvm::Value*, llvm::BasicBlock*>> branchVals;
	ir::Type*                                  resTy     = is_type_inferred() ? inferredType : nullptr;
	auto                                       currBlock = ctx->get_fn()->get_block();
	auto                                       resBlock  = ir::Block::create(ctx->get_fn(), currBlock->get_parent());
	resBlock->link_previous_block(currBlock);
	if (valTy->is_bool() || (valTy->is_native_type() || valTy->as_native_type()->is_native_bool())) {
		if (values.size() != 2) {
			ctx->Error("Inline matching a " + String(isRef ? "reference " : "value ") + "of type " +
			               ctx->color(valTy->to_string()) +
			               " requires exactly 2 expressions to match to the two possible values " + ctx->color("true") +
			               " and " + ctx->color("false") + ", but found " + ctx->color(std::to_string(values.size())) +
			               " expressions instead",
			           fileRange);
			ctx->irCtx->builder.CreatePHI(llvm::Type::getVoidTy(ctx->irCtx->llctx), 3);
		}
		expr->load_ghost_ref(ctx->irCtx->builder);
		auto cand = expr->get_llvm();
		if (isRef) {
			cand = ctx->irCtx->builder.CreateLoad(valTy->get_llvm_type(), cand);
		}
		auto trueBlock  = ir::Block::create(ctx->get_fn(), currBlock);
		auto falseBlock = ir::Block::create(ctx->get_fn(), currBlock);
		ctx->irCtx->builder.CreateCondBr(
		    valTy->is_bool()
		        ? cand
		        : ctx->irCtx->builder.CreateICmpNE(
		              cand, llvm::ConstantInt::get(llvm::cast<llvm::IntegerType>(valTy->get_llvm_type()), 0u)),
		    trueBlock->get_bb(), falseBlock->get_bb());
		trueBlock->set_active(ctx->irCtx->builder);
		auto tVal = values[0]->emit(ctx);
		if (resTy == nullptr) {
			resTy = tVal->get_pass_type();
		}
		if (not tVal->get_pass_type()->is_same(resTy)) {
			ctx->Error("The type inferred from scope for the result of this inline match is " +
			               ctx->color(resTy->to_string()) + " but this expression is of type " +
			               ctx->color(tVal->get_pass_type()->to_string()),
			           values[0]->fileRange);
		}
		tVal = ir::Logic::handle_pass_semantics(ctx, tVal->get_pass_type(), tVal, values[0]->fileRange);
		branchVals.push_back(std::make_pair(tVal->get_llvm(), trueBlock->get_bb()));
		(void)ir::add_branch(ctx->irCtx->builder, resBlock->get_bb());
		falseBlock->set_active(ctx->irCtx->builder);
		if (values[1]->has_type_inferrance()) {
			values[1]->as_type_inferrable()->set_inference_type(resTy);
		}
		auto fVal = values[1]->emit(ctx);
		if (not fVal->get_pass_type()->is_same(resTy)) {
			ctx->Error("The expression provided for the previous variant in this inline match is of type " +
			               ctx->color(resTy->to_string()) + ", but this expression is of type " +
			               ctx->color(fVal->get_pass_type()->to_string()),
			           values[1]->fileRange);
		}
		fVal = ir::Logic::handle_pass_semantics(ctx, fVal->get_pass_type(), fVal, values[1]->fileRange);
		branchVals.push_back(std::make_pair(fVal->get_llvm(), falseBlock->get_bb()));
		(void)ir::add_branch(ctx->irCtx->builder, resBlock->get_bb());
	} else if (valTy->is_choice()) {
		auto chTy = valTy->as_choice();
		if (values.size() != chTy->get_variant_count()) {
			ctx->Error("The expression being matched is of choice type " + ctx->color(chTy->to_string()) +
			               " which has " + ctx->color(std::to_string(chTy->get_variant_count())) + ", but only " +
			               ctx->color(std::to_string(values.size())) + " values are provided here",
			           fileRange);
		}
		expr->load_ghost_ref(ctx->irCtx->builder);
		auto cand = expr->get_llvm();
		if (isRef) {
			cand = ctx->irCtx->builder.CreateLoad(chTy->get_llvm_type(), cand);
		}
		for (usize i = 0; i < (values.size() - 1); i++) {
			auto* trueBlock  = ir::Block::create(ctx->fn, ctx->fn->get_block());
			auto* falseBlock = ir::Block::create(ctx->fn, ctx->fn->get_block());
			ctx->irCtx->builder.CreateCondBr(ctx->irCtx->builder.CreateICmpEQ(cand, chTy->get_value_at(i)),
			                                 trueBlock->get_bb(), falseBlock->get_bb());
			trueBlock->set_active(ctx->irCtx->builder);
			auto itVal = values[i]->emit(ctx);
			if (resTy == nullptr) {
				resTy = itVal->get_pass_type();
			}
			if (not itVal->get_pass_type()->is_same(resTy)) {
				ctx->Error((i == 0
				                ? "The type inferred from scope for the result of this inline match is "
				                : "Expressions provided for the previous variants in this inline match is of type ") +
				               ctx->color(resTy->to_string()) + ", but this expression is of type " +
				               ctx->color(itVal->get_pass_type()->to_string()),
				           values[i]->fileRange);
			}
			itVal = ir::Logic::handle_pass_semantics(ctx, itVal->get_pass_type(), itVal, values[i]->fileRange);
			branchVals.push_back(std::make_pair(itVal->get_llvm(), trueBlock->get_bb()));
			(void)ir::add_branch(ctx->irCtx->builder, resBlock->get_bb());
			falseBlock->set_active(ctx->irCtx->builder);
		}
		{ // Last case is automatically placed in the false block of the previous variant
			const auto i     = values.size() - 1;
			auto       itVal = values[i]->emit(ctx);
			if (not itVal->get_pass_type()->is_same(resTy)) {
				ctx->Error("Expressions provided for the previous variants in this inline match is of type " +
				               ctx->color(resTy->to_string()) + ", but this expression is of type " +
				               ctx->color(itVal->get_pass_type()->to_string()),
				           values[i]->fileRange);
			}
			itVal = ir::Logic::handle_pass_semantics(ctx, itVal->get_pass_type(), itVal, values[i]->fileRange);
			branchVals.push_back(std::make_pair(itVal->get_llvm(), ctx->fn->get_block()->get_bb()));
			(void)ir::add_branch(ctx->irCtx->builder, resBlock->get_bb());
		}
	} else if (valTy->is_mix()) {
		auto mxTy = valTy->as_mix();
		if (values.size() != mxTy->get_variant_count()) {
			ctx->Error("The expression being matched is of mix type " + ctx->color(mxTy->to_string()) + " which has " +
			               ctx->color(std::to_string(mxTy->get_variant_count())) + ", but only " +
			               ctx->color(std::to_string(values.size())) + " values are provided here",
			           fileRange);
		}
		llvm::Value* cand = nullptr;
		if (isRef || expr->is_ghost_ref()) {
			if (isRef) {
				expr->load_ghost_ref(ctx->irCtx->builder);
			}
			cand = ctx->irCtx->builder.CreateLoad(
			    llvm::cast<llvm::IntegerType>(llvm::cast<llvm::StructType>(mxTy->get_llvm_type())->getElementType(0u)),
			    ctx->irCtx->builder.CreateStructGEP(mxTy->get_llvm_type(), expr->get_llvm(), 0u));
		} else {
			cand = ctx->irCtx->builder.CreateExtractValue(expr->get_llvm(), {0u});
		}
		for (usize i = 0; i < (values.size() - 1); i++) {
			auto* trueBlock  = ir::Block::create(ctx->fn, ctx->fn->get_block());
			auto* falseBlock = ir::Block::create(ctx->fn, ctx->fn->get_block());
			ctx->irCtx->builder.CreateCondBr(
			    ctx->irCtx->builder.CreateICmpEQ(
			        cand,
			        llvm::ConstantInt::get(llvm::cast<llvm::IntegerType>(
			                                   llvm::cast<llvm::StructType>(mxTy->get_llvm_type())->getElementType(0u)),
			                               0u)),
			    trueBlock->get_bb(), falseBlock->get_bb());
			trueBlock->set_active(ctx->irCtx->builder);
			auto itVal = values[i]->emit(ctx);
			if (resTy == nullptr) {
				resTy = itVal->get_pass_type();
			}
			if (not itVal->get_pass_type()->is_same(resTy)) {
				ctx->Error((i == 0
				                ? "The type inferred from scope for the result of this inline match is "
				                : "Expressions provided for the previous variants in this inline match is of type ") +
				               ctx->color(resTy->to_string()) + ", but this expression is of type " +
				               ctx->color(itVal->get_pass_type()->to_string()),
				           values[i]->fileRange);
			}
			itVal = ir::Logic::handle_pass_semantics(ctx, itVal->get_pass_type(), itVal, values[i]->fileRange);
			branchVals.push_back(std::make_pair(itVal->get_llvm(), trueBlock->get_bb()));
			(void)ir::add_branch(ctx->irCtx->builder, resBlock->get_bb());
			falseBlock->set_active(ctx->irCtx->builder);
		}
		{ // Last case is automatically placed in the false block of the previous variant
			const auto i     = values.size() - 1;
			auto       itVal = values[i]->emit(ctx);
			if (not itVal->get_pass_type()->is_same(resTy)) {
				ctx->Error("Expressions provided for the previous variants in this inline match is of type " +
				               ctx->color(resTy->to_string()) + ", but this expression is of type " +
				               ctx->color(itVal->get_pass_type()->to_string()),
				           values[i]->fileRange);
			}
			itVal = ir::Logic::handle_pass_semantics(ctx, itVal->get_pass_type(), itVal, values[i]->fileRange);
			branchVals.push_back(std::make_pair(itVal->get_llvm(), ctx->fn->get_block()->get_bb()));
			(void)ir::add_branch(ctx->irCtx->builder, resBlock->get_bb());
		}
	} else if (valTy->is_maybe()) {
		auto mTy = valTy->as_maybe();
		if (values.size() != 2) {
			ctx->Error("The expression being matched is of the maybe type " + ctx->color(mTy->to_string()) +
			               " which requires 2 values to be provided, the first value to match to the value variant,"
			               " and the second value to match to the none variant",
			           fileRange);
		}
		llvm::Value* cand = nullptr;
		if (isRef || expr->is_ghost_ref()) {
			if (isRef) {
				expr->load_ghost_ref(ctx->irCtx->builder);
			}
			cand = ctx->irCtx->builder.CreateLoad(
			    llvm::Type::getInt1Ty(ctx->irCtx->llctx),
			    ctx->irCtx->builder.CreateStructGEP(mTy->get_llvm_type(), expr->get_llvm(), 0u));
		} else {
			cand = ctx->irCtx->builder.CreateExtractValue(expr->get_llvm(), {0u});
		}
		auto* trueBlock  = ir::Block::create(ctx->fn, ctx->fn->get_block());
		auto* falseBlock = ir::Block::create(ctx->fn, ctx->fn->get_block());
		ctx->irCtx->builder.CreateCondBr(cand, trueBlock->get_bb(), falseBlock->get_bb());
		trueBlock->set_active(ctx->irCtx->builder);
		auto trueVal = values[0]->emit(ctx);
		if (resTy == nullptr) {
			resTy = trueVal->get_pass_type();
		}
		if (not trueVal->get_pass_type()->is_same(resTy)) {
			ctx->Error("The type inferred from scope for the result of this inline match is " +
			               ctx->color(resTy->to_string()) + ", but this expression is of type " +
			               ctx->color(trueVal->get_pass_type()->to_string()),
			           values[0]->fileRange);
		}
		trueVal = ir::Logic::handle_pass_semantics(ctx, trueVal->get_pass_type(), trueVal, values[0]->fileRange);
		branchVals.push_back(std::make_pair(trueVal->get_llvm(), trueBlock->get_bb()));
		(void)ir::add_branch(ctx->irCtx->builder, resBlock->get_bb());
		falseBlock->set_active(ctx->irCtx->builder);
		auto falseVal = values[1]->emit(ctx);
		if (not falseVal->get_pass_type()->is_same(resTy)) {
			ctx->Error("Expression provided for the previous variant of this inline match is of type " +
			               ctx->color(resTy->to_string()) + ", but this expression is of type " +
			               ctx->color(falseVal->get_pass_type()->to_string()),
			           values[1]->fileRange);
		}
		falseVal = ir::Logic::handle_pass_semantics(ctx, falseVal->get_pass_type(), falseVal, values[1]->fileRange);
		branchVals.push_back(std::make_pair(falseVal->get_llvm(), falseBlock->get_bb()));
		(void)ir::add_branch(ctx->irCtx->builder, resBlock->get_bb());
	} else if (valTy->is_result()) {
		auto rTy = valTy->as_result();
		if (values.size() != 2) {
			ctx->Error("The expression being matched is of the result type " + ctx->color(rTy->to_string()) +
			               " which requires 2 values to be provided, the first value to match to the value variant,"
			               " and the second value to match to the error variant",
			           fileRange);
		}
		llvm::Value* cand = nullptr;
		if (isRef || expr->is_ghost_ref()) {
			if (isRef) {
				expr->load_ghost_ref(ctx->irCtx->builder);
			}
			cand = ctx->irCtx->builder.CreateLoad(
			    llvm::Type::getInt1Ty(ctx->irCtx->llctx),
			    ctx->irCtx->builder.CreateStructGEP(rTy->get_llvm_type(), expr->get_llvm(), 0u));
		} else {
			cand = ctx->irCtx->builder.CreateExtractValue(expr->get_llvm(), {0u});
		}
		auto* trueBlock  = ir::Block::create(ctx->fn, ctx->fn->get_block());
		auto* falseBlock = ir::Block::create(ctx->fn, ctx->fn->get_block());
		ctx->irCtx->builder.CreateCondBr(cand, trueBlock->get_bb(), falseBlock->get_bb());
		trueBlock->set_active(ctx->irCtx->builder);
		auto trueVal = values[0]->emit(ctx);
		if (resTy == nullptr) {
			resTy = trueVal->get_pass_type();
		}
		if (not trueVal->get_pass_type()->is_same(resTy)) {
			ctx->Error("The type inferred from scope for the result of this inline match is " +
			               ctx->color(resTy->to_string()) + ", but this expression is of type " +
			               ctx->color(trueVal->get_pass_type()->to_string()),
			           values[0]->fileRange);
		}
		trueVal = ir::Logic::handle_pass_semantics(ctx, trueVal->get_pass_type(), trueVal, values[0]->fileRange);
		branchVals.push_back(std::make_pair(trueVal->get_llvm(), trueBlock->get_bb()));
		(void)ir::add_branch(ctx->irCtx->builder, resBlock->get_bb());
		falseBlock->set_active(ctx->irCtx->builder);
		auto falseVal = values[1]->emit(ctx);
		if (not falseVal->get_pass_type()->is_same(resTy)) {
			ctx->Error("Expression provided for the previous variant of this inline match is of type " +
			               ctx->color(resTy->to_string()) + ", but this expression is of type " +
			               ctx->color(falseVal->get_pass_type()->to_string()),
			           values[1]->fileRange);
		}
		falseVal = ir::Logic::handle_pass_semantics(ctx, falseVal->get_pass_type(), falseVal, values[1]->fileRange);
		branchVals.push_back(std::make_pair(falseVal->get_llvm(), falseBlock->get_bb()));
		(void)ir::add_branch(ctx->irCtx->builder, resBlock->get_bb());
	} else if (valTy->is_ptr()) {
		auto ptrTy = valTy->as_ptr();
		if (ptrTy->is_non_nullable()) {
			ctx->Error(
			    "Cannot inline match a value of the non-nullable pointer type " + ctx->color(ptrTy->to_string()) +
			        ". Inline matching for pointers expects 2 values to match the first value to non-null variants and the"
			        " second value to the null variant, so expected an expression to be provided with a nullable pointer type",
			    expression->fileRange);
		}
		if (values.size() != 2) {
			ctx->Error("The expression being matched is of the pointer type " + ctx->color(valTy->to_string()) +
			               " which expects 2 values to be provided",
			           fileRange);
		}
		llvm::Value* cand = nullptr;
		if (ptrTy->is_multi()) {
			if (isRef || expr->is_ghost_ref()) {
				if (isRef) {
					expr->load_ghost_ref(ctx->irCtx->builder);
				}
				cand = ctx->irCtx->builder.CreateLoad(
				    llvm::cast<llvm::StructType>(ptrTy->get_llvm_type())->getElementType(0u),
				    ctx->irCtx->builder.CreateStructGEP(ptrTy->get_llvm_type(), expr->get_llvm(), 0u));
			} else {
				cand = ctx->irCtx->builder.CreateExtractValue(expr->get_llvm(), {0u});
			}
		} else {
			expr->load_ghost_ref(ctx->irCtx->builder);
			cand = expr->get_llvm();
			if (isRef) {
				cand = ctx->irCtx->builder.CreateLoad(ptrTy->get_llvm_type(), cand);
			}
		}
		auto* trueBlock  = ir::Block::create(ctx->fn, ctx->fn->get_block());
		auto* falseBlock = ir::Block::create(ctx->fn, ctx->fn->get_block());
		ctx->irCtx->builder.CreateCondBr(
		    ctx->irCtx->builder.CreateICmpNE(
		        ctx->irCtx->builder.CreatePtrDiff(
		            llvm::IntegerType::get(ctx->irCtx->llctx, 8u), cand,
		            llvm::ConstantPointerNull::get(
		                llvm::PointerType::get(ctx->irCtx->llctx, ctx->irCtx->dataLayout.getProgramAddressSpace()))),
		        llvm::ConstantInt::get(ir::NativeType::get_usize(ctx->irCtx)->get_llvm_type(), 0u)),
		    trueBlock->get_bb(), falseBlock->get_bb());
		trueBlock->set_active(ctx->irCtx->builder);
		auto trueVal = values[0]->emit(ctx);
		if (resTy == nullptr) {
			resTy = trueVal->get_pass_type();
		}
		if (not trueVal->get_pass_type()->is_same(resTy)) {
			ctx->Error("The type inferred from scope for the result of this inline match is " +
			               ctx->color(resTy->to_string()) + ", but this expression is of type " +
			               ctx->color(trueVal->get_pass_type()->to_string()),
			           values[0]->fileRange);
		}
		trueVal = ir::Logic::handle_pass_semantics(ctx, trueVal->get_pass_type(), trueVal, values[0]->fileRange);
		branchVals.push_back(std::make_pair(trueVal->get_llvm(), trueBlock->get_bb()));
		(void)ir::add_branch(ctx->irCtx->builder, resBlock->get_bb());
		falseBlock->set_active(ctx->irCtx->builder);
		auto falseVal = values[1]->emit(ctx);
		if (not falseVal->get_pass_type()->is_same(resTy)) {
			ctx->Error("Expression provided for the previous variant of this inline match is of type " +
			               ctx->color(resTy->to_string()) + ", but this expressin is of type " +
			               ctx->color(falseVal->get_pass_type()->to_string()),
			           values[1]->fileRange);
		}
		falseVal = ir::Logic::handle_pass_semantics(ctx, falseVal->get_pass_type(), falseVal, values[1]->fileRange);
		branchVals.push_back(std::make_pair(falseVal->get_llvm(), falseBlock->get_bb()));
		(void)ir::add_branch(ctx->irCtx->builder, resBlock->get_bb());
	}
	//
	resBlock->set_active(ctx->irCtx->builder);
	auto phi = ctx->irCtx->builder.CreatePHI(resTy->get_llvm_type(), branchVals.size());
	for (auto& it : branchVals) {
		phi->addIncoming(it.first, it.second);
	}
	return ir::Value::get(phi, resTy, not resTy->is_ref());
}

Json InlineMatch::to_json() const {
	Vec<JsonValue> valuesJSON;
	for (auto val : values) {
		valuesJSON.push_back(val->to_json());
	}
	return Json()
	    ._("nodeType", "inlineMatch")
	    ._("expression", expression->to_json())
	    ._("values", valuesJSON)
	    ._("fileRange", fileRange);
}

} // namespace qat::ast
