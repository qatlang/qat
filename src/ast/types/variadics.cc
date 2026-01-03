#include "./variadics.hpp"

namespace qat::ast {

ir::Variadics Variadics::to_ir(EmitCtx* ctx) const {
	auto      irKind = ir::VariadicsKind::NORMAL;
	ir::Type* irType = nullptr;
	switch (kind) {
		case VariadicKind::NORMAL: {
			break;
		}
		case VariadicKind::LEGACY: {
			irKind = ir::VariadicsKind::LEGACY;
			break;
		}
		case VariadicKind::TYPED: {
			irKind = ir::VariadicsKind::TYPED;
			irType = type->emit(ctx);
			if (not(irType->has_simple_copy() && irType->has_simple_move())) {
				ctx->Error("Typed variadics require a type with simple-copy and simple-move, but got the type " +
				               ctx->color(type->to_string()) + " instead which does not satisfy that constraint",
				           type->fileRange);
			}
			break;
		}
	}
	return ir::Variadics{.kind = irKind, .type = irType};
}

} // namespace qat::ast
