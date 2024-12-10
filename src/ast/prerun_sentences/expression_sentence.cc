#include "./expression_sentence.hpp"
#include "../expression.hpp"

namespace qat::ast {

void PrerunExpressionSentence::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect,
                                                   ir::EntityState* ent, EmitCtx* ctx) {
	expression->update_dependencies(phase, expect, ent, ctx);
}

void PrerunExpressionSentence::emit(EmitCtx* ctx) {
	auto expRes = expression->emit(ctx);
	if (not expRes->get_ir_type()->is_void()) {
		ctx->Error("Unused value of type " + ctx->color(expRes->get_ir_type()->to_string()) +
		               " here. If this is intentional and you wish to not use the value, please do " +
		               ctx->color("let ... = " + expression->to_string() + ".") + " to ignore the value",
		           fileRange);
	}
}

} // namespace qat::ast
