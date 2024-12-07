#include "./vector.hpp"
#include "../../IR/types/native_type.hpp"
#include "../../IR/types/vector.hpp"
#include "../expression.hpp"

#include <llvm/Analysis/ConstantFolding.h>

namespace qat::ast {

void VectorType::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
									 EmitCtx* ctx) {
	subType->update_dependencies(phase, ir::DependType::complete, ent, ctx);
	count->update_dependencies(phase, ir::DependType::complete, ent, ctx);
}

ir::Type* VectorType::emit(EmitCtx* ctx) {
	SHOW("Scalable has value " << scalable.has_value())
	auto subTy		 = subType->emit(ctx);
	auto usableSubTy = subTy->is_native_type() ? subTy->as_native_type()->get_subtype() : subTy;
	if (usableSubTy->is_integer() || usableSubTy->is_unsigned_integer() || usableSubTy->is_float()) {
		if (count->has_type_inferrance()) {
			count->as_type_inferrable()->set_inference_type(ir::UnsignedType::create(32u, ctx->irCtx));
		}
		auto num = count->emit(ctx);
		if (num->get_ir_type()->is_unsigned_integer() &&
			num->get_ir_type()->as_unsigned_integer()->get_bitwidth() == 32u) {
			auto numVal = llvm::cast<llvm::ConstantInt>(
				llvm::ConstantFoldConstant(num->get_llvm_constant(), ctx->irCtx->dataLayout.value()));
			if (numVal->isZero()) {
				ctx->Error("The number of elements of a vectorized type cannot be 0", count->fileRange);
			}
			return ir::VectorType::create(subTy, *numVal->getValue().getRawData(),
										  scalable.has_value() ? ir::VectorKind::scalable : ir::VectorKind::fixed,
										  ctx->irCtx);
		} else {
			ctx->Error("The number of elements of a vectorized type should be of " + ctx->color("u32") + "type",
					   count->fileRange);
		}
	} else {
		ctx->Error(
			"The element type of a vectorized type should be a signed or unsigned integer type, or a floating point type. The element type provided here is " +
				ctx->color(subTy->to_string()),
			subType->fileRange);
	}
	return nullptr;
}

Json VectorType::to_json() const {
	return Json()
		._("typeKind", "vector")
		._("subType", subType->to_json())
		._("count", count->to_json())
		._("isScalable", scalable.has_value())
		._("fileRange", fileRange);
}

String VectorType::to_string() const {
	return String("vec:[") + (scalable.has_value() ? "?, " : "") + count->to_string() + ", " + subType->to_string() +
		   "]";
}

} // namespace qat::ast
