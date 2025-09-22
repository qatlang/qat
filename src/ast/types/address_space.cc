#include "./address_space.hpp"
#include "../../IR/types/unsigned.hpp"
#include "../expression.hpp"

#include <llvm/Analysis/ConstantFolding.h>

namespace qat::ast {

ir::AddressSpace AddressSpace::to_ir(EmitCtx* ctx) const {
	if (name.value.empty()) {
		if (value->has_type_inferrance()) {
			value->as_type_inferrable()->set_inference_type(ir::UnsignedType::create(32u, ctx->irCtx));
		}
		auto val = value->emit(ctx);
		if (not val->get_ir_type()->is_same(ir::UnsignedType::create(32u, ctx->irCtx))) {
			ctx->Error(
			    "Expected an expression of type " + ctx->color(ir::UnsignedType::create(32u, ctx->irCtx)->to_string()) +
			        ", but got an expression of type " + ctx->color(val->get_ir_type()->to_string()) + " instead",
			    value->fileRange);
		}
		return ir::AddressSpace::from_value(
		    (u32)(*llvm::cast<llvm::ConstantInt>(
		               llvm::ConstantFoldConstant(val->get_llvm_constant(), ctx->irCtx->dataLayout))
		               ->getValue()
		               .getRawData()));
	} else {
		if (name.value == "program" || name.value == "global" || name.value == "local") {
			return ir::AddressSpace::from_name(name.value);
		} else {
			ctx->Error("The address-space " + ctx->color(name.value) + " is unrecognisable", name.range);
			std::unreachable();
		}
	}
}

String AddressSpace::to_string() const { return value ? ("of(" + value->to_string() + ")") : ("of:" + name.value); }

Json AddressSpace::to_json() const {
	return Json()
	    ._("name", name)
	    ._("hasValue", value != nullptr)
	    ._("value", value ? value->to_json() : JsonValue())
	    ._("range", fileRange ? fileRange->to_json_value() : JsonValue());
}

} // namespace qat::ast
