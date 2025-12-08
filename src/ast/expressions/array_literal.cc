#include "./array_literal.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/array.hpp"
#include "../types/qat_type.hpp"

#include <llvm/IR/Constants.h>

namespace qat::ast {

ir::Value* ArrayLiteral::emit(EmitCtx* ctx) {
	FnAtEnd   fnObj{[&] { createIn = nullptr; }};
	ir::Type* elemTy = nullptr;
	if (elemTyHint) {
		elemTy = elemTyHint->emit(ctx);
	}
	ir::ArrayType* arrTy = nullptr;
	if (is_type_inferred()) {
		if (not inferredType->is_array()) {
			ctx->Error("Type inferred from scope for this expression is " + ctx->color(inferredType->to_string()) +
			               ". This expression is an array literal.",
			           fileRange);
		}
		if (inferredType->as_array()->get_length() != values.size()) {
			auto infLen = inferredType->as_array()->get_length();
			ctx->Error("The length of the array type inferred is " + ctx->color(std::to_string(infLen)) + ", but " +
			               ((values.size() < infLen) ? "only " : "") + ctx->color(std::to_string(values.size())) +
			               " value" + ((values.size() > 1u) ? "s were" : "was") + " provided.",
			           fileRange);
		}
		arrTy = inferredType->as_array();
		if (elemTy) {
			if (not arrTy->get_element_type()->is_same(elemTy)) {
				ctx->Error(
				    "The hint provided about the type of the element of this array literal is " +
				        ctx->color(elemTy->to_string()) +
				        ", which does not match with the element type of the array type as inferred from scope, which is " +
				        ctx->color(inferredType->to_string()),
				    fileRange);
			}
		} else {
			elemTy = arrTy->get_element_type();
		}
	}
	Vec<ir::Value*> valsIR;
	bool            areAllValsConstant = true;
	for (usize i = 0; i < values.size(); i++) {
		if (values[i]->has_type_inferrance() && elemTy) {
			values[i]->as_type_inferrable()->set_inference_type(elemTy);
		}
		valsIR.push_back(values[i]->emit(ctx));
		if (elemTy == nullptr) {
			elemTy = valsIR[0]->get_pass_type();
		}
		if (not valsIR.back()->is_prerun_value()) {
			areAllValsConstant = false;
			valsIR[i] =
			    ir::Logic::handle_pass_semantics(ctx, valsIR[i]->get_pass_type(), valsIR[i], values[i]->fileRange);
		}
		if (not valsIR[i]->get_ir_type()->is_same(elemTy)) {
			ctx->Error("The expected type of this expression is " + ctx->color(elemTy->to_string()) +
			               " but the provided expression is of type " +
			               ctx->color(valsIR[i]->get_ir_type()->to_string()) + ". These do not match",
			           values.at(i)->fileRange);
		}
	}
	if (not elemTy) {
		ctx->Error("This is an empty array literal. Either the type of this expression should be inferred from scope,"
		           " or a hint about the element of the array should be provided. You can use the syntax " +
		               ctx->color("[]:[ElementType]") + " to provide hint about the element type of the array",
		           fileRange);
	}
	if (not arrTy) {
		arrTy = ir::ArrayType::get(elemTy, values.size(), ctx->irCtx->llctx);
	}
	if (not values.empty()) {
		llvm::Constant* constVal = nullptr;
		if (areAllValsConstant) {
			Vec<llvm::Constant*> valsConst;
			for (auto* v : valsIR) {
				valsConst.push_back(v->get_llvm_constant());
			}
			constVal =
			    llvm::ConstantArray::get(llvm::ArrayType::get(elemTy->get_llvm_type(), valsIR.size()), valsConst);
			if (not isLocalDecl() && not canCreateIn()) {
				return ir::PrerunValue::get(constVal, arrTy)->with_range(fileRange);
			}
		}
		if (canCreateIn()) {
			if (not type_check_create_in(arrTy)) {
				ctx->Error(
				    "Tried to optimise the array literal by creating in-place, but the type of in-place creation is " +
				        ctx->color(createIn->get_ir_type()->to_string()) + ". Expected the type to be " +
				        ctx->color(arrTy->to_string()) + " instead",
				    fileRange);
			}
		} else if (isLocalDecl() || (not areAllValsConstant)) {
			createIn = ctx->get_fn()->get_block()->new_local(
			    irName.has_value() ? irName->value : ctx->get_fn()->get_random_alloca_name(), arrTy, isVar, ctx->irCtx,
			    irName.has_value() ? irName->range : fileRange);
		} else {
			return ir::PrerunValue::get(constVal, arrTy)->with_range(fileRange);
		}
		auto* elemPtr = ctx->irCtx->builder.CreateInBoundsGEP(
		    arrTy->get_llvm_type(), createIn->get_llvm(),
		    {llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->irCtx->llctx), 0u),
		     llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->irCtx->llctx), 0u)});
		ctx->irCtx->builder.CreateStore(valsIR.at(0)->get_llvm(), elemPtr);
		for (usize i = 1; i < valsIR.size(); i++) {
			elemPtr = ctx->irCtx->builder.CreateInBoundsGEP(
			    elemTy->get_llvm_type(), elemPtr,
			    llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->irCtx->llctx), 1u));
			ctx->irCtx->builder.CreateStore(valsIR.at(i)->get_llvm(), elemPtr);
		}
		return get_creation_result(ctx->irCtx, arrTy, fileRange);
	} else {
		if (isLocalDecl()) {
			createIn = ctx->get_fn()->get_block()->new_local(
			    irName.has_value() ? irName->value : ctx->get_fn()->get_random_alloca_name(), arrTy, isVar, ctx->irCtx,
			    irName.has_value() ? irName->range : fileRange);
		}
		if (canCreateIn()) {
			ctx->irCtx->builder.CreateStore(
			    llvm::ConstantArray::get(llvm::cast<llvm::ArrayType>(arrTy->get_llvm_type()), {}),
			    createIn->get_llvm());
			return get_creation_result(ctx->irCtx, arrTy, fileRange);
		} else {
			return ir::PrerunValue::get(
			           llvm::ConstantArray::get(llvm::cast<llvm::ArrayType>(arrTy->get_llvm_type()), {}), arrTy)
			    ->with_range(fileRange);
		}
	}
}

Json ArrayLiteral::to_json() const {
	Vec<JsonValue> vals;
	for (auto* exp : values) {
		vals.push_back(exp->to_json());
	}
	return Json()._("nodeType", "arrayLiteral")._("values", vals);
}

} // namespace qat::ast
