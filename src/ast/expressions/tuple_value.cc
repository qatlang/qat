#include "./tuple_value.hpp"
#include "../../IR/logic.hpp"

#include <llvm/IR/DerivedTypes.h>

namespace qat::ast {

ir::Value* TupleValue::emit(EmitCtx* ctx) {
	ir::TupleType* tupleTy = nullptr;
	SHOW("Checking type inferrence")
	if (is_type_inferred()) {
		if (not inferredType->is_tuple()) {
			ctx->Error("The inferred type for this tuple expression is " + ctx->color(inferredType->to_string()) +
			               ", which is not a tuple type",
			           fileRange);
		}
		tupleTy = is_type_inferred() ? inferredType->as_tuple() : nullptr;
		if (inferredType->as_tuple()->getSubTypeCount() != members.size()) {
			ctx->Error("Expected the type of this tuple to be " + ctx->color(inferredType->to_string()) + " with " +
			               ctx->color(std::to_string(inferredType->as_tuple()->getSubTypeCount())) + " members. But " +
			               ctx->color(std::to_string(members.size())) + " values were provided",
			           fileRange);
		}
	}
	SHOW("Getting member types and member IR values")
	Vec<ir::Type*>  memTys;
	Vec<ir::Value*> memsIR;
	bool            isAllMemsPre = true;
	for (usize i = 0; i < members.size(); i++) {
		auto* expMemTy = tupleTy ? tupleTy->getSubtypeAt(i) : nullptr;
		auto* mem      = members.at(i);
		if (expMemTy) {
			if (mem->has_type_inferrance()) {
				mem->as_type_inferrable()->set_inference_type(expMemTy->is_ref() ? expMemTy->as_ref()->get_subtype()
				                                                                 : expMemTy);
			}
		}
		auto* memRes = mem->emit(ctx);
		if (expMemTy && not expMemTy->is_same(memRes->get_pass_type())) {
			ctx->Error("This expression was expected to be of type " + ctx->color(expMemTy->to_string()) +
			               ", but got an expression of type " + ctx->color(memRes->get_pass_type()->to_string()) +
			               " instead",
			           mem->fileRange);
		}
		if (not tupleTy) {
			memTys.push_back(memRes->get_pass_type());
		}
		auto valRes = ir::Logic::handle_pass_semantics(ctx, expMemTy ? expMemTy : memRes->get_pass_type(), memRes,
		                                               mem->fileRange);
		if (not valRes->is_prerun_value()) {
			isAllMemsPre = false;
		}
		memsIR.push_back(valRes);
	}
	if (not tupleTy) {
		tupleTy = ir::TupleType::get(memTys, isPacked, ctx->irCtx->llctx);
	}
	llvm::Constant* constVal = nullptr;
	if (isAllMemsPre) {
		Vec<llvm::Constant*> memConst;
		for (auto it : memsIR) {
			memConst.push_back(it->get_llvm_constant());
		}
		constVal = llvm::ConstantStruct::get(llvm::cast<llvm::StructType>(tupleTy->get_llvm_type()), memConst);
	}
	if (isLocalDecl() || not isAllMemsPre) {
		createIn = ctx->get_fn()->get_block()->new_local(
		    irName.has_value() ? irName.value().value : utils::uid_string(), tupleTy, irName.has_value() ? isVar : true,
		    irName.has_value() ? irName.value().range : fileRange);
	}
	if (constVal) {
		if (canCreateIn()) {
			ctx->irCtx->builder.CreateStore(constVal, createIn->get_llvm());
			return get_creation_result(ctx->irCtx, tupleTy, fileRange);
		} else {
			return ir::PrerunValue::get(constVal, tupleTy)->with_range(fileRange);
		}
	} else {
		for (usize i = 0; i < memsIR.size(); i++) {
			ctx->irCtx->builder.CreateStore(
			    memsIR.at(i)->get_llvm(),
			    ctx->irCtx->builder.CreateStructGEP(tupleTy->get_llvm_type(), createIn->get_llvm(), i));
		}
		return get_creation_result(ctx->irCtx, tupleTy, fileRange);
	}
}

Json TupleValue::to_json() const {
	Vec<JsonValue> mems;
	for (auto mem : members) {
		mems.push_back(mem->to_json());
	}
	return Json()._("nodeType", "tupleValue")._("isPacked", isPacked)._("members", mems)._("fileRange", fileRange);
}

} // namespace qat::ast
