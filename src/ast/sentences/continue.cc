#include "./continue.hpp"
#include "../../IR/control_flow.hpp"
#include "../../IR/types/void.hpp"

namespace qat::ast {

ir::Value* Continue::emit(EmitCtx* ctx) {
	if (ctx->loopsInfo.empty()) {
		ctx->Error("Continue sentence is not present inside any loop block", fileRange);
	} else {
		if (tag.has_value()) {
			for (usize i = (ctx->loopsInfo.size() - 1); i >= 0; i--) {
				auto& loopInfo = ctx->loopsInfo.at(i);
				if (loopInfo.name.has_value() && (loopInfo.name->value == tag->value)) {
					if (loopInfo.type == LoopType::INFINITE) {
						ir::destroy_locals_from(ctx->irCtx, loopInfo.mainBlock);
						return ir::Value::get(ir::add_branch(ctx->irCtx->builder, loopInfo.mainBlock->get_bb()),
											  ir::VoidType::get(ctx->irCtx->llctx), false);
					} else {
						ir::destroy_locals_from(ctx->irCtx, loopInfo.mainBlock);
						return ir::Value::get(ir::add_branch(ctx->irCtx->builder, loopInfo.condBlock->get_bb()),
											  ir::VoidType::get(ctx->irCtx->llctx), false);
					}
					return nullptr;
				}
			}
			ctx->Error("The provided tag " + ctx->color(tag->value) +
						   " does not match the tag of any parent loops or switches",
					   fileRange);
		} else {
			if (ctx->loopsInfo.size() == 1) {
				if (ctx->loopsInfo.front().type == LoopType::INFINITE) {
					ir::destroy_locals_from(ctx->irCtx, ctx->loopsInfo.front().mainBlock);
					return ir::Value::get(
						ir::add_branch(ctx->irCtx->builder, ctx->loopsInfo.front().mainBlock->get_bb()),
						ir::VoidType::get(ctx->irCtx->llctx), false);
				} else {
					ir::destroy_locals_from(ctx->irCtx, ctx->loopsInfo.front().mainBlock);
					return ir::Value::get(
						ir::add_branch(ctx->irCtx->builder, ctx->loopsInfo.front().condBlock->get_bb()),
						ir::VoidType::get(ctx->irCtx->llctx), false);
				}
			} else {
				ctx->Error(
					"It is compulsory to provide the tagged name of the loop in a continue sentence, if there are nested loops",
					fileRange);
			}
		}
	}
	return nullptr;
}

Json Continue::to_json() const {
	return Json()._("hasTag", tag.has_value())._("tag", tag.has_value() ? tag.value() : JsonValue());
}

} // namespace qat::ast
