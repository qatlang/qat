#include "./get_intrinsic.hpp"
#include "../../IR/stdlib.hpp"
#include "../../IR/types/function.hpp"
#include "../../IR/types/vector.hpp"
#include "../../IR/value.hpp"

#include <llvm/Analysis/ConstantFolding.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Intrinsics.h>

namespace qat::ast {

ir::Value* GetIntrinsic::emit(EmitCtx* ctx) {
	if (args.empty()) {
		ctx->Error("Expected at least one value to be provided. The first value should be the id of the intrinsic",
		           fileRange);
	}
	auto nameIR = args[0]->emit(ctx);
	if (not ir::StdLib::is_std_lib_found()) {
		ctx->Error("The standard library could not be found, and hence cannot retrieve intrinsic", fileRange);
	}
	if (not ir::StdLib::stdLib->has_choice_type("Intrinsic", AccessInfo::GetPrivileged())) {
		ctx->Error(
		    "The choice type " + ctx->color("Intrinsic") +
		        " is not found in the standard library, and hence the ID of the intrinsic could not be determined",
		    fileRange);
	}
	auto intrChTy = ir::StdLib::stdLib->get_choice_type("Intrinsic", AccessInfo::GetPrivileged());
	if (nameIR->get_ir_type()->is_same(intrChTy)) {
		auto nmVal = (u32)(*llvm::cast<llvm::ConstantInt>(nameIR->get_llvm_constant())->getValue().getRawData());
		if (nmVal == (u32)IntrinsicID::llvm_matrix_multiply) {
			if (args.size() == 6) {
				auto firstVal  = args[1]->emit(ctx);
				auto secondVal = args[2]->emit(ctx);
				if (not firstVal->get_ir_type()->is_typed()) {
					ctx->Error("Expected a type here, got an expression of type " +
					               ctx->color(firstVal->get_ir_type()->to_string()),
					           args[1]->fileRange);
				}
				if (not secondVal->get_ir_type()->is_typed()) {
					ctx->Error("Expected a type here, got an expression of type " +
					               ctx->color(firstVal->get_ir_type()->to_string()),
					           args[2]->fileRange);
				}
				auto firstTy = ir::TypeInfo::get_for(firstVal->get_llvm_constant())->type;
				if (not(firstTy->is_vector() && firstTy->as_vector()->is_fixed())) {
					ctx->Error("The first type should be a fixed vector type, got " + ctx->color(firstTy->to_string()) +
					               " instead",
					           args[1]->fileRange);
				}
				auto secondTy = ir::TypeInfo::get_for(secondVal->get_llvm_constant())->type;
				if (not(secondTy->is_vector() && secondTy->as_vector()->is_fixed())) {
					ctx->Error("The second type should be a fixed vector type, got " +
					               ctx->color(secondVal->get_ir_type()->to_string()) + " instead",
					           args[2]->fileRange);
				}
				auto oneTy = firstTy->as_vector();
				auto twoTy = secondTy->as_vector();
				if (not oneTy->get_element_type()->is_same(twoTy->get_element_type())) {
					ctx->Error("The first vector type has an element type of " +
					               ctx->color(oneTy->get_element_type()->to_string()) +
					               " but the second vector type has an element type of " +
					               ctx->color(twoTy->get_element_type()->to_string()),
					           fileRange);
				}
				if (args[3]->has_type_inferrance()) {
					args[3]->as_type_inferrable()->set_inference_type(ir::UnsignedType::create(32u, ctx->irCtx));
				}
				if (args[4]->has_type_inferrance()) {
					args[4]->as_type_inferrable()->set_inference_type(ir::UnsignedType::create(32u, ctx->irCtx));
				}
				if (args[5]->has_type_inferrance()) {
					args[5]->as_type_inferrable()->set_inference_type(ir::UnsignedType::create(32u, ctx->irCtx));
				}
				auto thirdVal  = args[3]->emit(ctx);
				auto fourthVal = args[4]->emit(ctx);
				auto fifthVal  = args[5]->emit(ctx);
				auto checkFn   = [&](ir::PrerunValue* value, FileRange range) {
                    if (not(value->get_ir_type()->is_unsigned_integer() &&
                            (value->get_ir_type()->as_unsigned_integer()->get_bitwidth() == 32u))) {
                        ctx->Error("This value is expected to be of type " + ctx->color("u32") +
						                 ". Got an expression of type " + ctx->color(value->get_ir_type()->to_string()),
						             range);
                    }
				};
				checkFn(thirdVal, args[3]->fileRange);
				checkFn(fourthVal, args[4]->fileRange);
				checkFn(fifthVal, args[5]->fileRange);
				auto oneMulRes =
				    llvm::ConstantExpr::getMul(thirdVal->get_llvm_constant(), fourthVal->get_llvm_constant());
				if (not llvm::cast<llvm::ConstantInt>(
				            llvm::ConstantFoldCompareInstruction(
				                llvm::CmpInst::ICMP_EQ,
				                llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx->irCtx->llctx), oneTy->get_count()),
				                oneMulRes))
				            ->getValue()
				            .getBoolValue()) {
					ctx->Error("The first type provided is " + ctx->color(oneTy->to_string()) +
					               " but the product of the 3rd and 4th values is " +
					               ctx->color(std::to_string(
					                   *llvm::cast<llvm::ConstantInt>(
					                        llvm::ConstantFoldConstant(oneMulRes, ctx->irCtx->dataLayout.value()))
					                        ->getValue()
					                        .getRawData())) +
					               ". The element count does not match",
					           fileRange);
				}
				auto twoMulRes =
				    llvm::ConstantExpr::getMul(fourthVal->get_llvm_constant(), fifthVal->get_llvm_constant());
				if (not llvm::cast<llvm::ConstantInt>(
				            llvm::ConstantFoldCompareInstruction(
				                llvm::CmpInst::ICMP_EQ,
				                llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx->irCtx->llctx), twoTy->get_count()),
				                twoMulRes))
				            ->getValue()
				            .getBoolValue()) {
					ctx->Error("The second type provided is " + ctx->color(oneTy->to_string()) +
					               " but the product of the 4th and 5th values is " +
					               ctx->color(std::to_string(
					                   *llvm::cast<llvm::ConstantInt>(
					                        llvm::ConstantFoldConstant(oneMulRes, ctx->irCtx->dataLayout.value()))
					                        ->getValue()
					                        .getRawData())) +
					               ". The element count does not match",
					           fileRange);
				}
				auto retTy = ir::VectorType::create(
				    oneTy->get_element_type(),
				    *llvm::cast<llvm::ConstantInt>(
				         llvm::ConstantFoldConstant(
				             llvm::ConstantExpr::getMul(thirdVal->get_llvm_constant(), fifthVal->get_llvm_constant()),
				             ctx->irCtx->dataLayout.value()))
				         ->getValue()
				         .getRawData(),
				    ir::VectorKind::fixed, ctx->irCtx);
				auto mod    = ctx->mod;
				auto intrFn = llvm::Intrinsic::getDeclaration(
				    mod->get_llvm_module(), llvm::Intrinsic::matrix_multiply,
				    {retTy->get_llvm_type(), oneTy->get_llvm_type(), twoTy->get_llvm_type()});
				auto fnTy =
				    ir::FunctionType::create(ir::ReturnType::get(retTy),
				                             {
				                                 ir::ArgumentType::create_normal(oneTy, None, false),
				                                 ir::ArgumentType::create_normal(twoTy, None, false),
				                                 ir::ArgumentType::create_normal(thirdVal->get_ir_type(), None, false),
				                                 ir::ArgumentType::create_normal(thirdVal->get_ir_type(), None, false),
				                                 ir::ArgumentType::create_normal(thirdVal->get_ir_type(), None, false),
				                             },
				                             ctx->irCtx->llctx);
				return ir::Value::get(intrFn, fnTy, false);
			} else {
				ctx->Error(
				    "This intrinsic requires 5 parameters to be provided after the Intrinsic ID. The first two parameters are the underlying vectors types of the matrices, the 3rd parameter is the number of rows of the first matrix, the 4th parameter is the number of columns of the first matrix which is equal to the number of rows of the second matrix, and the 5th parameter is the number of columns of the second matrix",
				    fileRange);
			}
		} else {
			ctx->Error("This intrinsic is not supported", args[0]->fileRange);
		}
	} else {
		ctx->Error("The first parameter should be the ID of the intrinsic of type " + ctx->color(intrChTy->to_string()),
		           args[0]->fileRange);
	}
}

Json GetIntrinsic::to_json() const {
	Vec<JsonValue> vals;
	for (auto arg : args) {
		vals.push_back(arg->to_json());
	}
	return Json()._("parameters", vals)._("fileRange", fileRange);
}

} // namespace qat::ast
