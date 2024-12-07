#include "./loop_if.hpp"
#include "../../IR/control_flow.hpp"

namespace qat::ast {

ir::Value* LoopIf::emit(EmitCtx* ctx) {
	String uniq;
	if (tag.has_value()) {
		uniq = tag->value;
		for (const auto& info : ctx->loopsInfo) {
			if (info.name.has_value() && (info.name->value == uniq)) {
				ctx->Error("The tag provided for this loop is already used by another loop", tag->range,
						   Pair<String, FileRange>{"The existing tag was found here", info.name->range});
			}
			if (info.secondaryName.has_value() && (info.secondaryName->value == uniq)) {
				ctx->Error("The tag provided for this loop is already used by another loop", tag->range,
						   Pair<String, FileRange>{"The existing tag was found here", info.secondaryName->range});
			}
		}
		for (const auto& brek : ctx->breakables) {
			if (brek.tag.has_value() && (brek.tag->value == tag->value)) {
				ctx->Error("The tag provided for the loop is already used by another " +
							   ctx->color(brek.type_to_string()),
						   tag->range, Pair<String, FileRange>{"The existing tag was found here", brek.tag->range});
			}
		}
		auto block = ctx->get_fn()->get_block();
		if (block->has_value(tag->value)) {
			ctx->Error("There already exists another local value with the same name as this tag", tag->range,
					   block->get_value(tag->value)->has_associated_range()
						   ? Maybe<Pair<String, FileRange>>(
								 {"The local value was found here", block->get_value(tag->value)->get_file_range()})
						   : None);
		}
	} else {
		uniq = utils::unique_id();
	}
	ir::Value* cond = isDoAndLoop ? nullptr : condition->emit(ctx);
	if (cond == nullptr || cond->get_ir_type()->is_bool() ||
		(cond->get_ir_type()->is_reference() && cond->get_ir_type()->as_reference()->get_subtype()->is_bool())) {
		auto*		 fun	   = ctx->get_fn();
		auto*		 trueBlock = new ir::Block(fun, fun->get_block());
		auto*		 condBlock = new ir::Block(fun, fun->get_block());
		auto*		 restBlock = new ir::Block(fun, fun->get_block());
		llvm::Value* llCond	   = nullptr;
		if (isDoAndLoop) {
			(void)ir::add_branch(ctx->irCtx->builder, trueBlock->get_bb());
		} else {
			llCond = cond->get_llvm();
			cond->load_ghost_reference(ctx->irCtx->builder);
			if (cond->get_ir_type()->is_reference()) {
				cond->load_ghost_reference(ctx->irCtx->builder);
				llCond = ctx->irCtx->builder.CreateLoad(
					cond->get_ir_type()->as_reference()->get_subtype()->get_llvm_type(), cond->get_llvm());
			}
			ctx->irCtx->builder.CreateCondBr(llCond, trueBlock->get_bb(), restBlock->get_bb());
		}
		ctx->loopsInfo.push_back(LoopInfo(tag, trueBlock, condBlock, restBlock, nullptr,
										  isDoAndLoop ? LoopType::DO_WHILE : LoopType::WHILE));
		ctx->breakables.push_back(Breakable(BreakableType::loop, tag, restBlock, trueBlock));
		trueBlock->set_active(ctx->irCtx->builder);
		emit_sentences(sentences, ctx);
		trueBlock->destroy_locals(ctx);
		ctx->loopsInfo.pop_back();
		ctx->breakables.pop_back();
		(void)ir::add_branch(ctx->irCtx->builder, condBlock->get_bb());
		condBlock->set_active(ctx->irCtx->builder);
		cond = condition->emit(ctx);
		if (isDoAndLoop) {
			if (!cond->get_ir_type()->is_bool() && !(cond->get_ir_type()->is_reference() &&
													 cond->get_ir_type()->as_reference()->get_subtype()->is_bool())) {
				ctx->Error(
					"The condition for the " + ctx->color("do-while") + " loop should be of " + ctx->color("bool") +
						" type. Got an expression of type " + ctx->color(cond->get_ir_type()->to_string()) +
						" instead. Please check if you forgot to add a comparison, or made a mistake in the expression",
					condition->fileRange);
			}
		}
		cond->load_ghost_reference(ctx->irCtx->builder);
		if (cond->get_ir_type()->is_reference()) {
			llCond = ctx->irCtx->builder.CreateLoad(cond->get_ir_type()->as_reference()->get_subtype()->get_llvm_type(),
													cond->get_llvm());
		} else {
			llCond = cond->get_llvm();
		}
		ctx->irCtx->builder.CreateCondBr(llCond, trueBlock->get_bb(), restBlock->get_bb());
		restBlock->set_active(ctx->irCtx->builder);
	} else {
		ctx->Error("The expression used for the condition of " + ctx->color(isDoAndLoop ? "do-while" : "while") +
					   " loop should be of " + ctx->color("bool") + " type. Got an expression of type " +
					   ctx->color(cond->get_ir_type()->to_string()) +
					   " instead.Please check if you forgot to add a comparison, or made a mistake in the expression",
				   condition->fileRange);
	}
	return nullptr;
}

Json LoopIf::to_json() const {
	Vec<JsonValue> snts;
	for (auto* snt : sentences) {
		snts.push_back(snt->to_json());
	}
	return Json()
		._("nodeType", "loopWhile")
		._("condition", condition->to_json())
		._("sentences", snts)
		._("fileRange", fileRange);
}

} // namespace qat::ast
