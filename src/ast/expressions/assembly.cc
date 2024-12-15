#include "./assembly.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/function.hpp"
#include "../../IR/types/void.hpp"
#include "../expression.hpp"
#include "../types/qat_type.hpp"

#include <llvm/Analysis/ConstantFolding.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/Support/Error.h>

namespace qat::ast {

void AssemblyBlock::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent,
                                        EmitCtx* ctx) {
	if (functionType) {
		functionType->update_dependencies(phase, ir::DependType::complete, ent, ctx);
	}
	asmValue->update_dependencies(phase, ir::DependType::complete, ent, ctx);
	for (auto arg : arguments) {
		arg->update_dependencies(phase, ir::DependType::complete, ent, ctx);
	}
	if (clobbers) {
		clobbers->update_dependencies(phase, ir::DependType::complete, ent, ctx);
	}
	if (volatileExp) {
		volatileExp->update_dependencies(phase, ir::DependType::complete, ent, ctx);
	}
}

ir::Value* AssemblyBlock::emit(EmitCtx* ctx) {
	if (ctx->has_fn()) {
		ir::FunctionType* asmFnTy = nullptr;
		if (functionType) {
			auto irTy = functionType->emit(ctx);
			SHOW("AssemblyBlock :: Emitted function type")
			if (!irTy->is_function()) {
				ctx->Error("Expected a function type to represent the nature of the assembly code",
				           functionType->fileRange);
			}
			asmFnTy = irTy->as_function();
		} else {
			asmFnTy = ir::FunctionType::create(ir::ReturnType::get(ir::VoidType::get(ctx->irCtx->llctx)), {},
			                                   ctx->irCtx->llctx);
		}
		auto asmStrExp = asmValue->emit(ctx);
		SHOW("ASM string emitted")
		if (!asmStrExp->get_ir_type()->is_string_slice()) {
			ctx->Error("Expected this expression to be of type " + ctx->color("str"), asmValue->fileRange);
		}
		auto asmStr = ir::StringSliceType::value_to_string(asmStrExp);
		if (asmFnTy->get_argument_count() != arguments.size()) {
			if (asmFnTy->get_argument_count() == 0) {
				ctx->Error("Expected no arguments for this assembly code", argsRange);
			} else if (arguments.size() == 0) {
				ctx->Error("Expected " + std::to_string(asmFnTy->get_argument_count()) +
				               " argument values, but none were provided",
				           argsRange);
			} else {
				ctx->Error("Expected " + String((asmFnTy->get_argument_count() < arguments.size()) ? "only " : "") +
				               std::to_string(asmFnTy->get_argument_count()) +
				               " argument values to be provided to pass to the assembly code. But " +
				               ((arguments.size() < asmFnTy->get_argument_count()) ? "only " : "") +
				               std::to_string(arguments.size()) + " values were provided",
				           argsRange);
			}
		}
		SHOW("Checked arg count")
		Vec<ir::Value*> argsIR;
		for (usize i = 0; i < arguments.size(); i++) {
			if (arguments[i]->has_type_inferrance()) {
				arguments[i]->as_type_inferrable()->set_inference_type(asmFnTy->get_argument_type_at(i)->get_type());
			}
			argsIR.push_back(arguments[i]->emit(ctx));
		}
		SHOW("Args emitted")
		Vec<llvm::Value*> argsLLVM;
		for (usize i = 0; i < argsIR.size(); i++) {
			argsIR[i] = ir::Logic::handle_pass_semantics(ctx, asmFnTy->get_argument_type_at(i)->get_type(), argsIR[i],
			                                             arguments[i]->fileRange);
			argsLLVM.push_back(argsIR[i]->get_llvm());
		}
		SHOW("Args handled")
		String clobberString;
		auto   clobberExpr = clobbers->emit(ctx);
		SHOW("Clobber str emitted")
		if (clobberExpr->get_ir_type()->is_array() &&
		    clobberExpr->get_ir_type()->as_array()->get_element_type()->is_string_slice()) {
			SHOW("ClobberExpr: " << clobberExpr->get_ir_type()->to_prerun_generic_string(clobberExpr).value())
			auto clobConst = llvm::cast<llvm::ConstantArray>(
			    llvm::ConstantFoldConstant(clobberExpr->get_llvm_constant(), ctx->irCtx->dataLayout.value()));
			auto itemCount = clobberExpr->get_ir_type()->as_array()->get_length();
			for (usize i = 0; i < itemCount; i++) {
				auto item = clobConst->getAggregateElement(i);
				clobberString += ir::StringSliceType::value_to_string(
				    ir::PrerunValue::get(item, ir::StringSliceType::get(ctx->irCtx)));
				if (i != (itemCount - 1)) {
					clobberString += ",";
				}
			}
		} else {
			ctx->Error("The list of constraints is expected to be of " + ctx->color("[n]str") + " type",
			           clobbers->fileRange);
		}
		SHOW("Got clobber string")
		if (!clobberString.empty()) {
			auto verifyRes =
			    llvm::InlineAsm::verify(llvm::cast<llvm::FunctionType>(asmFnTy->get_llvm_type()), clobberString);
			if (verifyRes) {
				SHOW("Getting error string")
				auto errStr = llvm::toString(std::move(verifyRes));
				SHOW("Got error string")
				ctx->Error("The resultant constraint string " + ctx->color(clobberString) +
				               " leads to the error: " + errStr,
				           clobbers->fileRange);
			}
		}
		bool isVolatileValue = true;
		if (volatileExp) {
			auto vExp = volatileExp->emit(ctx);
			if (!vExp->get_ir_type()->is_bool()) {
				ctx->Error("The expression to determine volatility of the assembly block is expected to be of " +
				               ctx->color("bool") + " type",
				           volatileExp->fileRange);
			}
			isVolatileValue = llvm::cast<llvm::ConstantInt>(vExp->get_llvm_constant())->getValue().getBoolValue();
		}
		auto inlineASM = llvm::InlineAsm::get(llvm::cast<llvm::FunctionType>(asmFnTy->get_llvm_type()), asmStr,
		                                      clobberString, isVolatileValue, false, llvm::InlineAsm::AD_ATT, false);
		SHOW("Got llvm::InlineAsm")
		return ir::Value::get(ctx->irCtx->builder.CreateCall(inlineASM, argsLLVM),
		                      asmFnTy->get_return_type()->get_type(), false)
		    ->with_range(fileRange);
	} else {
		ctx->Error("Expected this block to be present within a normal function", fileRange);
	}
	return nullptr;
}

Json AssemblyBlock::to_json() const {
	return Json()._("nodeType", "assemblyBlock")._("asmValue", asmValue->to_json())._("fileRange", fileRange);
}

} // namespace qat::ast
