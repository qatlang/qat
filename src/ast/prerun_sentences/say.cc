#include "./say.hpp"
#include "../../IR/types/integer.hpp"
#include "../expression.hpp"

namespace qat::ast {

void PrerunSay::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
                                    EmitCtx* ctx) {
	for (auto* val : values) {
		UPDATE_DEPS(val);
	}
}

void PrerunSay::emit(EmitCtx* ctx) {
	for (auto val : values) {
		auto irVal = val->emit(ctx);
		auto irTy  = irVal->get_ir_type();
		if (irVal->get_ir_type()->is_text()) {
			std::cout << irTy->as_text()->value_to_string(irVal);
		} else {
			auto strVal = irTy->to_prerun_generic_string(irVal);
			if (not strVal.has_value()) {
				ctx->Error("Could not convert this prerun value of type " + ctx->color(irTy->to_string()) +
				               " to an internal string",
				           val->fileRange);
			}
			std::cout << strVal.value();
		}
	}
	std::cout << '\n';
}

} // namespace qat::ast
