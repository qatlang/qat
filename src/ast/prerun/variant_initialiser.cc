#include "./variant_initialiser.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/toggle.hpp"

#include <llvm/Analysis/ConstantFolding.h>

namespace qat::ast {

ir::PrerunValue* PrerunVariantInitialiser::emit(EmitCtx* ctx) {
	SHOW("Prerun Mix/Choice type initialiser")
	if (not type && not is_type_inferred()) {
		ctx->Error("No type is provided for this expression, and no type could be inferred from scope", fileRange);
	}
	auto* typeEmit = type ? type.emit(ctx) : inferredType;
	if (type && is_type_inferred() && not typeEmit->is_same(inferredType)) {
		ctx->Error("The type provided is " + ctx->color(typeEmit->to_string()) +
		               " but the type inferred from scope is " + ctx->color(inferredType->to_string()),
		           type.get_range());
	}
	if (typeEmit->is_mix()) {
		auto* mixTy = typeEmit->as_mix();
		if (not mixTy->can_be_prerun()) {
			ctx->Error("Mix type " + ctx->color(mixTy->to_string()) + " cannot be used in a prerun expression",
			           fileRange);
		}
		auto subRes = mixTy->has_variant_with_name(subName.value);
		SHOW("Subtype check")
		if (subRes.first) {
			if (subRes.second) {
				if (not expression.has_value()) {
					ctx->Error("Variant " + ctx->color(subName.value) + " of mix type " +
					               ctx->color(mixTy->get_full_name()) + " expects a value to be associated with it",
					           fileRange);
				}
			} else {
				if (expression.has_value()) {
					ctx->Error("Variant " + ctx->color(subName.value) + " of mix type " +
					               ctx->color(mixTy->get_full_name()) + " cannot have any value associated with it",
					           fileRange);
				}
			}
			llvm::Constant* exp = nullptr;
			if (subRes.second) {
				auto* typ = mixTy->get_variant_with_name(subName.value);
				if (expression.value()->has_type_inferrance()) {
					expression.value()->as_type_inferrable()->set_inference_type(typ);
				}
				auto* expEmit = expression.value()->emit(ctx);
				if (typ->is_same(expEmit->get_ir_type())) {
					exp           = expEmit->get_llvm_constant();
					auto typeBits = (u64)ctx->irCtx->dataLayout.getTypeStoreSizeInBits(typ->get_llvm_type());
					exp           = llvm::ConstantFoldCastInstruction(llvm::CastInst::CastOps::BitCast, exp,
					                                                  llvm::Type::getIntNTy(ctx->irCtx->llctx, typeBits));
				} else {
					ctx->Error("The expected type is " + ctx->color(typ->to_string()) +
					               ", but the expression is of type " + ctx->color(expEmit->get_ir_type()->to_string()),
					           expression.value()->fileRange);
				}
			} else {
				exp = llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->irCtx->llctx, mixTy->get_data_bitwidth()), 0u);
			}
			auto* index = llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->irCtx->llctx, mixTy->get_tag_bitwidth()),
			                                     mixTy->get_index_of(subName.value));
			return ir::PrerunValue::get(
			    llvm::ConstantStruct::get(llvm::cast<llvm::StructType>(mixTy->get_llvm_type()), {index, exp}), mixTy);
		} else {
			ctx->Error("No field named " + ctx->color(subName.value) + " is present inside mix type " +
			               ctx->color(mixTy->get_full_name()),
			           fileRange);
		}
	} else if (typeEmit->is_choice()) {
		if (expression.has_value()) {
			ctx->Error("An expression is provided here, but the recognised type is a choice type: " +
			               ctx->color(typeEmit->to_string()) + ". Please remove the expression or check the logic here",
			           expression.value()->fileRange);
		}
		auto* chTy = typeEmit->as_choice();
		if (chTy->has_field(subName.value)) {
			return ir::PrerunValue::get(chTy->get_value_for(subName.value), chTy);
		} else {
			ctx->Error("Choice type " + ctx->color(chTy->to_string()) + " does not have a variant named " +
			               ctx->color(subName.value),
			           subName.range);
		}
	} else if (typeEmit->is_toggle()) {
		auto tgTy = typeEmit->as_toggle();
		if (not tgTy->has_variant(subName.value)) {
			ctx->Error("Toggle type " + ctx->color(tgTy->to_string()) + " does not have a variant named " +
			               ctx->color(subName.value),
			           subName.range);
		}
		const bool isDefaultVar = tgTy->is_default_variant(subName.value);
		auto       varType      = tgTy->get_variant_type_of(subName.value);

		if (not expression.has_value()) {
			ctx->Error("The variant " + ctx->color(subName.value) + " of the toggle type " +
			               ctx->color(typeEmit->to_string()) + " expects an expression of type " +
			               ctx->color(varType->to_string()) + ". Please change this expression to " +
			               ctx->color(type.to_string() + "::" + subName.value + "(variantExpression)"),
			           subName.range);
		}
		auto subVal = expression.value()->emit(ctx);
		if (not subVal->get_pass_type()->is_same(varType)) {
			ctx->Error("The variant " + ctx->color(subName.value) + " of the toggle type " +
			               ctx->color(tgTy->to_string()) + " have the type " + ctx->color(varType->to_string()) +
			               " associated with it, but found an expression of type " +
			               ctx->color(subVal->get_pass_type()->to_string()),
			           expression.value()->fileRange);
		}
		if (isDefaultVar) {
			Vec<llvm::Constant*> toggleElems;
			toggleElems.reserve(2);
			toggleElems.push_back(subVal->get_llvm_constant());
			if (llvm::cast<llvm::StructType>(tgTy->get_llvm_type())->getNumElements() == 2) {
				toggleElems.push_back(llvm::ConstantAggregateZero::get(
				    llvm::cast<llvm::StructType>(tgTy->get_llvm_type())->getElementType(1)));
			}
			return ir::PrerunValue::get(
			    llvm::ConstantStruct::get(llvm::cast<llvm::StructType>(tgTy->get_llvm_type()), toggleElems), tgTy);
		} else {
			auto                 toggleSize  = (usize)ctx->irCtx->dataLayout.getTypeStoreSize(tgTy->get_llvm_type());
			auto                 variantSize = (usize)ctx->irCtx->dataLayout.getTypeStoreSize(varType->get_llvm_type());
			auto                 variantArray = llvm::cast<llvm::ConstantArray>(llvm::ConstantFoldConstant(
                llvm::ConstantExpr::getBitCast(
                    subVal->as_prerun()->get_llvm_constant(),
                    llvm::ArrayType::get(llvm::Type::getInt8Ty(ctx->irCtx->llctx), variantSize)),
                ctx->irCtx->dataLayout));
			Vec<llvm::Constant*> toggleArrElems(toggleSize,
			                                    llvm::ConstantInt::get(llvm::Type::getInt8Ty(ctx->irCtx->llctx), 0u));
			for (usize i = 0; i < variantSize; i++) {
				toggleArrElems[i] = variantArray->getAggregateElement(i);
			}
			return ir::PrerunValue::get(
			    llvm::ConstantFoldConstant(
			        llvm::ConstantExpr::getBitCast(
			            llvm::ConstantArray::get(
			                llvm::ArrayType::get(llvm::Type::getInt8Ty(ctx->irCtx->llctx), toggleSize), toggleArrElems),
			            tgTy->get_llvm_type()),
			        ctx->irCtx->dataLayout),
			    tgTy);
		}
	} else {
		ctx->Error(ctx->color(typeEmit->to_string()) +
		               " is not a mix type or a choice type and hence cannot be used here",
		           fileRange);
	}
	return nullptr;
}

String PrerunVariantInitialiser::to_string() const {
	return type.to_string() + "::" + subName.value +
	       (expression.has_value() ? ("(" + expression.value()->to_string() + ")") : "");
}

Json PrerunVariantInitialiser::to_json() const {
	return Json()
	    ._("nodeType", "prerunMixOrChoiceInit")
	    ._("hasType", (bool)type)
	    ._("type", type.to_json_value())
	    ._("subName", subName)
	    ._("hasExpression", expression.has_value())
	    ._("expression", expression.has_value() ? expression.value()->to_json() : JsonValue())
	    ._("fileRange", fileRange);
}

} // namespace qat::ast
