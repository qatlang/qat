#include "./continue.hpp"
#include "./internal_exceptions.hpp"

namespace qat::ast {

void PrerunContinue::emit(EmitCtx* ctx) {
	if (ctx->get_pre_call_state()->loopsInfo.empty()) {
		ctx->Error("This " + ctx->color("continue") + " is not inside any loop. Please check your logic", fileRange);
	}
	if (tag.has_value()) {
		for (auto& inf : ctx->get_pre_call_state()->loopsInfo) {
			if (inf.tag.has_value() && inf.tag->value == tag->value) {
				throw InternalPrerunContinue{tag->value};
			}
		}
	} else if (ctx->get_pre_call_state()->loopsInfo.size() == 1) {
		throw InternalPrerunContinue();
	} else {
		ctx->Error("Found multiple nested loops. Please provide the tag of the loop for continuing", fileRange);
	}
}

} // namespace qat::ast
