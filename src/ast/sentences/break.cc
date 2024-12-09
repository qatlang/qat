#include "./break.hpp"
#include "../../IR/control_flow.hpp"
#include "../../IR/types/void.hpp"

namespace qat::ast {

ir::Value* Break::emit(EmitCtx* ctx) {
	if (ctx->breakables.empty()) {
		ctx->Error(ctx->color("break") + " sentence is not present inside any " + ctx->color("loop") + " or " +
		               ctx->color("match") + " blocks",
		           fileRange);
	} else {
		if (tag.has_value()) {
			for (auto& brk : ctx->breakables) {
				if (brk.tag.has_value() && (brk.tag->value == tag->value)) {
					ir::destroy_locals_from(ctx->irCtx, brk.trueBlock);
					return ir::Value::get(ir::add_branch(ctx->irCtx->builder, brk.restBlock->get_bb()),
					                      ir::VoidType::get(ctx->irCtx->llctx), false);
				}
			}
			ctx->Error("The provided tag " + ctx->color(tag->value) +
			               " does not match the tag of any parent loop or match blocks",
			           tag->range);
		} else {
			if (ctx->breakables.size() == 1) {
				ir::destroy_locals_from(ctx->irCtx, ctx->breakables.front().trueBlock);
				return ir::Value::get(ir::add_branch(ctx->irCtx->builder, ctx->breakables.front().restBlock->get_bb()),
				                      ir::VoidType::get(ctx->irCtx->llctx), false);
			} else {
				ctx->Error("It is compulsory to provide the tagged name of the " + ctx->color("loop") + " or " +
				               ctx->color("match") + " in a " + ctx->color("break") +
				               " sentence, if there is a nesting of loops or matches",
				           fileRange);
			}
		}
	}
	return nullptr;
}

Json Break::to_json() const {
	return Json()._("hasTag", tag.has_value())._("tag", tag.has_value() ? tag->value : JsonValue());
}

} // namespace qat::ast
