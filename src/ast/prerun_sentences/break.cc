#include "./break.hpp"
#include "./internal_exceptions.hpp"

namespace qat::ast {

void PrerunBreak::emit(EmitCtx* ctx) {
	if (ctx->get_pre_call_state()->loopsInfo.empty()) {
		ctx->Error("This " + ctx->color("break") + " is not inside a loop. Please check your logic", fileRange);
	}
	if (tag.has_value()) {
		for (auto& inf : ctx->get_pre_call_state()->loopsInfo) {
			if (inf.tag.has_value() && inf.tag->value == tag->value) {
				throw InternalPrerunBreak{tag->value};
			}
		}
	} else if (ctx->get_pre_call_state()->loopsInfo.size() == 1) {
		throw InternalPrerunBreak();
	} else {
		ctx->Error("Found multiple nested loops. Please provide the tag of the loop to break from", fileRange);
	}
}

} // namespace qat::ast
