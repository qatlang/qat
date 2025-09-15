#include "./intrinsic.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/metalib.hpp"
#include "../../IR/stdlib.hpp"
#include "../../IR/types/pointer.hpp"
#include "../../IR/types/unsigned.hpp"
#include "../../IR/types/vector.hpp"
#include "../../IR/types/void.hpp"
#include "../../IR/value.hpp"

#include <llvm/Analysis/ConstantFolding.h>
#include <llvm/IR/ConstantFold.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Intrinsics.h>

#define INTRINSIC_CHOICE_NAME "Intrinsics"

namespace qat::ast {

ir::Value* MetaIntrinsic::emit(EmitCtx* ctx) {
	auto intrChTy = ir::MetaLib::get_intrinsic_id(ctx->irCtx);
	if (name->has_type_inferrance()) {
		name->as_type_inferrable()->set_inference_type(intrChTy);
	}
	auto nameIR = name->emit(ctx);
	if (not nameIR->get_ir_type()->is_same(intrChTy)) {
		ctx->Error("The first generic parameter should be the ID of the intrinsic of type " +
		               ctx->color(intrChTy->to_string()),
		           genArgs[0]->fileRange);
	}
	auto nmVal = (IntrinsicID)(*llvm::cast<llvm::ConstantInt>(nameIR->get_llvm_constant())->getValue().getRawData());
	if (nmVal == IntrinsicID::matrix_multiply) {
		if (genArgs.size() != 2u) {
			ctx->Error("This intrinsic requires 2 generic parameters to be provided after the Intrinsic ID. "
			           "The generic parameters are the underlying vectors types of the matrices to be multiplied",
			           fileRange);
		}
		auto firstVal  = genArgs[0]->emit(ctx);
		auto secondVal = genArgs[1]->emit(ctx);
		if (not firstVal->get_ir_type()->is_typed()) {
			ctx->Error("Expected a type here, got an expression of type " +
			               ctx->color(firstVal->get_ir_type()->to_string()),
			           genArgs[0]->fileRange);
		}
		if (not secondVal->get_ir_type()->is_typed()) {
			ctx->Error("Expected a type here, got an expression of type " +
			               ctx->color(firstVal->get_ir_type()->to_string()),
			           genArgs[1]->fileRange);
		}
		auto firstTy = ir::TypeInfo::get_for(firstVal->get_llvm_constant())->type;
		if (not(firstTy->is_vector() && firstTy->as_vector()->is_fixed())) {
			ctx->Error("The first type should be a fixed vector type, got " + ctx->color(firstTy->to_string()) +
			               " instead",
			           genArgs[0]->fileRange);
		}
		auto secondTy = ir::TypeInfo::get_for(secondVal->get_llvm_constant())->type;
		if (not(secondTy->is_vector() && secondTy->as_vector()->is_fixed())) {
			ctx->Error("The second type should be a fixed vector type, got " +
			               ctx->color(secondVal->get_ir_type()->to_string()) + " instead",
			           genArgs[1]->fileRange);
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
		if (arguments.size() != 5u) {
			ctx->Error("This intrinsic call requires 5 arguments to be provided. The first two arguments are the"
			           " vector values that represent the matrices to be multiplied, having the types " +
			               ctx->color(firstTy->to_string()) + " and " + ctx->color(secondTy->to_string()) +
			               ". The third argument is the number of rows of the first matrix. The fourth argument"
			               " is the number of columns of the first matrix and always matches the number of rows"
			               " of the second matrix. The fifth argument is the number of columns of the second matrix",
			           fileRange);
		}
		auto u32Ty = ir::UnsignedType::create(32u, ctx->irCtx);
		if (arguments[2]->has_type_inferrance()) {
			arguments[2]->as_type_inferrable()->set_inference_type(u32Ty);
		}
		if (arguments[3]->has_type_inferrance()) {
			arguments[3]->as_type_inferrable()->set_inference_type(u32Ty);
		}
		if (arguments[4]->has_type_inferrance()) {
			arguments[4]->as_type_inferrable()->set_inference_type(u32Ty);
		}
		auto thirdVal  = arguments[2]->emit(ctx);
		auto fourthVal = arguments[3]->emit(ctx);
		auto fifthVal  = arguments[4]->emit(ctx);
		auto checkFn   = [&](ir::Value* value, FileRangePtr range) {
            if (not(value->get_ir_type()->is_unsigned() &&
                    (value->get_ir_type()->as_unsigned()->get_bitwidth() == 32u))) {
                ctx->Error("This value is expected to be of type " + ctx->color("u32") +
				                 ". Got an expression of type " + ctx->color(value->get_ir_type()->to_string()),
				             range);
            }
		};
		checkFn(thirdVal, arguments[2]->fileRange);
		checkFn(fourthVal, arguments[3]->fileRange);
		checkFn(fifthVal, arguments[4]->fileRange);
		if (thirdVal->is_prerun_value() && fourthVal->is_prerun_value()) {
			auto oneMulRes = llvm::ConstantExpr::getMul(thirdVal->get_llvm_constant(), fourthVal->get_llvm_constant());
			if (not llvm::cast<llvm::ConstantInt>(
			            llvm::ConstantFoldCompareInstruction(
			                llvm::CmpInst::ICMP_EQ,
			                llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx->irCtx->llctx), oneTy->get_count()),
			                oneMulRes))
			            ->getValue()
			            .getBoolValue()) {
				ctx->Error(
				    "The first vector type provided is " + ctx->color(oneTy->to_string()) +
				        " but the product of the 3rd and 4th values is " +
				        ctx->color(std::to_string(*llvm::cast<llvm::ConstantInt>(
				                                       llvm::ConstantFoldConstant(oneMulRes, ctx->irCtx->dataLayout))
				                                       ->getValue()
				                                       .getRawData())) +
				        ". The element count does not match",
				    fileRange);
			}
		}
		if (fourthVal->is_prerun_value() && fifthVal->is_prerun_value()) {
			auto twoMulRes = llvm::ConstantExpr::getMul(fourthVal->get_llvm_constant(), fifthVal->get_llvm_constant());
			if (not llvm::cast<llvm::ConstantInt>(
			            llvm::ConstantFoldCompareInstruction(
			                llvm::CmpInst::ICMP_EQ,
			                llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx->irCtx->llctx), twoTy->get_count()),
			                twoMulRes))
			            ->getValue()
			            .getBoolValue()) {
				ctx->Error(
				    "The second vector type provided is " + ctx->color(oneTy->to_string()) +
				        " but the product of the 4th and 5th values is " +
				        ctx->color(std::to_string(*llvm::cast<llvm::ConstantInt>(
				                                       llvm::ConstantFoldConstant(twoMulRes, ctx->irCtx->dataLayout))
				                                       ->getValue()
				                                       .getRawData())) +
				        ". The element count does not match",
				    fileRange);
			}
		}
		auto retTy = ir::VectorType::create(
		    oneTy->get_element_type(),
		    *llvm::cast<llvm::ConstantInt>(
		         llvm::ConstantFoldConstant(
		             llvm::ConstantExpr::getMul(thirdVal->get_llvm_constant(), fifthVal->get_llvm_constant()),
		             ctx->irCtx->dataLayout))
		         ->getValue()
		         .getRawData(),
		    ir::VectorKind::fixed, ctx->irCtx);
		auto mod = ctx->mod;
		mod->set_matrix_intrinsic_used();
		auto intrFn = llvm::Intrinsic::getOrInsertDeclaration(
		    mod->get_llvm_module(), llvm::Intrinsic::matrix_multiply,
		    {retTy->get_llvm_type(), oneTy->get_llvm_type(), twoTy->get_llvm_type()});
		auto firstArg  = arguments[0]->emit(ctx);
		auto secondArg = arguments[1]->emit(ctx);
		if (not firstArg->get_pass_type()->is_same(firstTy)) {
			ctx->Error("Expected an expression of type " + ctx->color(firstTy->to_string()) +
			               " here, but got an expression of type " + ctx->color(firstArg->get_pass_type()->to_string()),
			           arguments[0]->fileRange);
		}
		if (not secondArg->get_pass_type()->is_same(secondTy)) {
			ctx->Error("Expected an expression of type " + ctx->color(secondTy->to_string()) +
			               " here, but got an expression of type " +
			               ctx->color(secondArg->get_pass_type()->to_string()),
			           arguments[1]->fileRange);
		}
		firstArg = ir::Logic::handle_pass_semantics(ctx, firstArg->get_pass_type(), firstArg, arguments[0]->fileRange);
		secondArg =
		    ir::Logic::handle_pass_semantics(ctx, secondArg->get_pass_type(), secondArg, arguments[1]->fileRange);
		return ir::Value::get(
		    ctx->irCtx->builder.CreateCall(intrFn->getFunctionType(), intrFn,
		                                   {firstArg->get_llvm(), secondArg->get_llvm(), thirdVal->get_llvm(),
		                                    fourthVal->get_llvm(), fifthVal->get_llvm()}),
		    retTy, true);
	} else if (nmVal == IntrinsicID::matrix_transpose) {
		if (genArgs.size() != 1) {
			ctx->Error(
			    "This intrinsic requires 1 generic parameter to be provided after the Intrinsic ID. The generic parameter"
			    " is the underlying vector type of the matrix",
			    fileRange);
		}
		auto firstVal = genArgs[0]->emit(ctx);
		if (not firstVal->get_ir_type()->is_typed()) {
			ctx->Error("Expected a type here, but got an expression of type " +
			               ctx->color(firstVal->get_ir_type()->to_string()),
			           genArgs[0]->fileRange);
		}
		auto firstTy = ir::TypeInfo::get_for(firstVal->get_llvm_constant())->type;
		if (not(firstTy->is_vector() && firstTy->as_vector()->is_fixed())) {
			ctx->Error("The generic parameter should be a fixed vector type, got " + ctx->color(firstTy->to_string()) +
			               " instead",
			           genArgs[0]->fileRange);
		}
		if (arguments.size() != 3u) {
			ctx->Error(
			    "This intrinsic call expects 3 arguments to be provided. The first argument is the vector value of the matrix"
			    " to be transposed. The second argument is the number of rows and the third argument is the number of columns",
			    fileRange);
		}
		auto secondVal = arguments[1]->emit(ctx);
		if (not(secondVal->get_ir_type()->is_unsigned() &&
		        (secondVal->get_ir_type()->as_unsigned()->get_bitwidth() == 32u))) {
			ctx->Error("This value is expected to be of type " + ctx->color("u32") + ". Got an expression of type " +
			               ctx->color(secondVal->get_ir_type()->to_string()),
			           arguments[1]->fileRange);
		}
		auto thirdVal = arguments[2]->emit(ctx);
		if (not(thirdVal->get_ir_type()->is_unsigned() &&
		        (thirdVal->get_ir_type()->as_unsigned()->get_bitwidth() == 32u))) {
			ctx->Error("This value is expected to be of type " + ctx->color("u32") + ". Got an expression of type " +
			               ctx->color(thirdVal->get_ir_type()->to_string()),
			           arguments[2]->fileRange);
		}
		if (secondVal->is_prerun_value() && thirdVal->is_prerun_value()) {
			auto oneMulRes = llvm::ConstantExpr::getMul(secondVal->get_llvm_constant(), thirdVal->get_llvm_constant());
			if (not llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldCompareInstruction(
			                                          llvm::CmpInst::ICMP_EQ,
			                                          llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx->irCtx->llctx),
			                                                                 firstTy->as_vector()->get_count()),
			                                          oneMulRes))
			            ->getValue()
			            .getBoolValue()) {
				ctx->Error(
				    "The vector type provided is " + ctx->color(firstTy->to_string()) +
				        " but the product of the number of rows and columns is " +
				        ctx->color(std::to_string(*llvm::cast<llvm::ConstantInt>(
				                                       llvm::ConstantFoldConstant(oneMulRes, ctx->irCtx->dataLayout))
				                                       ->getValue()
				                                       .getRawData())) +
				        ". The element count does not match",
				    fileRange);
			}
		}
		ctx->mod->set_matrix_intrinsic_used();
		auto intrFn = llvm::Intrinsic::getOrInsertDeclaration(
		    ctx->mod->get_llvm_module(), llvm::Intrinsic::matrix_transpose, {firstTy->get_llvm_type()});
		auto argVal = arguments[0]->emit(ctx);
		if (not argVal->get_pass_type()->is_same(firstTy)) {
			ctx->Error("Expected an expression of type " + ctx->color(firstTy->to_string()) +
			               " here, but got an expression of type " + ctx->color(argVal->get_pass_type()->to_string()) +
			               " instead",
			           fileRange);
		}
		argVal = ir::Logic::handle_pass_semantics(ctx, argVal->get_pass_type(), argVal, arguments[0]->fileRange);
		return ir::Value::get(
		    ctx->irCtx->builder.CreateCall(intrFn->getFunctionType(), intrFn,
		                                   {argVal->get_llvm(), secondVal->get_llvm(), thirdVal->get_llvm()}),
		    firstTy, true);
	} else if (nmVal == IntrinsicID::matrix_column_major_load) {
		if (genArgs.size() != 1u) {
			ctx->Error("This intrinsic requires 1 generic parameter to be provided, which should be "
			           "the underlying vector type of the matrix type. But could not find any generic parameters here",
			           fileRange);
		}
		auto firstVal = genArgs[0]->emit(ctx);
		if (not firstVal->get_ir_type()->is_typed()) {
			ctx->Error("Expected a type here, but got an expression of type " +
			               ctx->color(firstVal->get_ir_type()->to_string()),
			           genArgs[0]->fileRange);
		}
		auto firstTy = ir::TypeInfo::get_for(firstVal->get_llvm_constant())->type;
		if (not(firstTy->is_vector() && firstTy->as_vector()->is_fixed())) {
			ctx->Error("The generic parameter should be a fixed vector type, but got " +
			               ctx->color(firstTy->to_string()) + " instead",
			           genArgs[0]->fileRange);
		}
		if (arguments.size() != 5u) {
			ctx->Error(
			    "This intrinsic call requires 5 arguments to be provided. The first argument is the pointer to the"
			    " matrix data, the second argument is the stride which is the gap between the first element of"
			    " consecutive columns and is always greater than or equal to the number of rows. The third argument is a " +
			        ctx->color("bool") +
			        " value that determines the volatile nature of the memory access. The fourth"
			        " argument is the number of rows and the fifth argument is the number of columns",
			    fileRange);
		}
		auto ptrVal = arguments[0]->emit(ctx);
		ptrVal      = ir::Logic::handle_pass_semantics(ctx, ptrVal->get_pass_type(), ptrVal, arguments[0]->fileRange);
		auto voidPtrTy = ir::PtrType::get(false, ir::VoidType::get(ctx->irCtx->llctx), true,
		                                  ir::PtrOwner::of_anonymous(), false, ctx->irCtx);
		auto vecPtrTy  = ir::PtrType::get(false, firstTy, true, ir::PtrOwner::of_anonymous(), false, ctx->irCtx);
		if (not ptrVal->get_ir_type()->is_same(voidPtrTy) && not ptrVal->get_ir_type()->is_same(vecPtrTy)) {
			ctx->Error("The first argument is expected to be of type " + ctx->color(voidPtrTy->to_string()) + " or " +
			               ctx->color(vecPtrTy->to_string()) + ", but got an expression of type " +
			               ctx->color(ptrVal->get_ir_type()->to_string()) + " instead",
			           arguments[0]->fileRange);
		}
		auto strideVal = arguments[1]->emit(ctx);
		strideVal =
		    ir::Logic::handle_pass_semantics(ctx, strideVal->get_pass_type(), strideVal, arguments[1]->fileRange);
		if (not strideVal->get_ir_type()->is_same(ir::UnsignedType::create(64u, ctx->irCtx))) {
			ctx->Error("The second argument is expected to be of type " +
			               ctx->color(ir::UnsignedType::create(64u, ctx->irCtx)->to_string()) +
			               ", but got an expression of type " + ctx->color(strideVal->get_ir_type()->to_string()) +
			               " instead",
			           arguments[1]->fileRange);
		}
		auto volVal = arguments[2]->emit(ctx);
		volVal      = ir::Logic::handle_pass_semantics(ctx, volVal->get_pass_type(), volVal, arguments[2]->fileRange);
		if (not volVal->get_ir_type()->is_same(ir::UnsignedType::create_bool(ctx->irCtx))) {
			ctx->Error("The third argument is expected to be of type " +
			               ctx->color(ir::UnsignedType::create_bool(ctx->irCtx)->to_string()) +
			               " but got an expression of type " + ctx->color(volVal->get_ir_type()->to_string()) +
			               " instead",
			           arguments[2]->fileRange);
		}
		auto u32Ty  = ir::UnsignedType::create(32u, ctx->irCtx);
		auto rowVal = arguments[3]->emit(ctx);
		rowVal      = ir::Logic::handle_pass_semantics(ctx, rowVal->get_pass_type(), rowVal, arguments[3]->fileRange);
		if (not rowVal->get_ir_type()->is_same(u32Ty)) {
			ctx->Error("The fourth argument is expected to be of type " + ctx->color(u32Ty->to_string()) +
			               ", but got an expression of type " + ctx->color(rowVal->get_ir_type()->to_string()) +
			               " instead",
			           arguments[3]->fileRange);
		}
		auto colVal = arguments[4]->emit(ctx);
		colVal      = ir::Logic::handle_pass_semantics(ctx, colVal->get_pass_type(), colVal, arguments[4]->fileRange);
		if (not colVal->get_ir_type()->is_same(u32Ty)) {
			ctx->Error("The fifth argument is expected to be of type " + ctx->color(u32Ty->to_string()) +
			               ", but got an expression of type " + ctx->color(colVal->get_ir_type()->to_string()) +
			               " instead",
			           arguments[4]->fileRange);
		}
		if (rowVal->is_prerun_value() && strideVal->is_prerun_value()) {
			if (not llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldCompareInstruction(
			                                          llvm::CmpInst::ICMP_UGE, strideVal->get_llvm_constant(),
			                                          llvm::ConstantFoldCastInstruction(
			                                              llvm::ICmpInst::CastOps::ZExt, rowVal->get_llvm_constant(),
			                                              strideVal->get_ir_type()->get_llvm_type())))
			            ->getValue()
			            .getBoolValue()) {
				ctx->Error("The second argument is the stride which is expected to be"
				           " greater than or equal to the number of rows of the matrix",
				           arguments[1]->fileRange);
			}
		}
		if (rowVal->is_prerun_value() && colVal->is_prerun_value()) {
			auto mulRes = llvm::ConstantExpr::getMul(rowVal->get_llvm_constant(), colVal->get_llvm_constant());
			if (not llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldCompareInstruction(
			                                          llvm::CmpInst::ICMP_EQ,
			                                          llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx->irCtx->llctx),
			                                                                 firstTy->as_vector()->get_count()),
			                                          mulRes))
			            ->getValue()
			            .getBoolValue()) {
				ctx->Error(
				    "The vector type provided is " + ctx->color(firstTy->to_string()) +
				        " but the product of the number of rows and columns is " +
				        ctx->color(std::to_string(
				            *llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldConstant(mulRes, ctx->irCtx->dataLayout))
				                 ->getValue()
				                 .getRawData())) +
				        ". The element count does not match",
				    fileRange);
			}
		}
		ctx->mod->set_matrix_intrinsic_used();
		auto intrFn = llvm::Intrinsic::getOrInsertDeclaration(
		    ctx->mod->get_llvm_module(), llvm::Intrinsic::matrix_column_major_load, {firstTy->get_llvm_type()});
		return ir::Value::get(
		    ctx->irCtx->builder.CreateCall(intrFn->getFunctionType(), intrFn,
		                                   {ptrVal->get_llvm(), strideVal->get_llvm(), volVal->get_llvm(),
		                                    rowVal->get_llvm(), colVal->get_llvm()}),
		    firstTy, true);
	} else if (nmVal == IntrinsicID::matrix_column_major_store) {
		if (genArgs.size() != 1u) {
			ctx->Error("This intrinsic requires 1 generic parameter to be provided, which should be "
			           "the underlying vector type of the matrix type. But could not find any generic parameters here",
			           fileRange);
		}
		auto firstVal = genArgs[0]->emit(ctx);
		if (not firstVal->get_ir_type()->is_typed()) {
			ctx->Error("Expected a type here, but got an expression of type " +
			               ctx->color(firstVal->get_ir_type()->to_string()),
			           genArgs[0]->fileRange);
		}
		auto firstTy = ir::TypeInfo::get_for(firstVal->get_llvm_constant())->type;
		if (not(firstTy->is_vector() && firstTy->as_vector()->is_fixed())) {
			ctx->Error("The generic parameter should be a fixed vector type, but got " +
			               ctx->color(firstTy->to_string()) + " instead",
			           genArgs[0]->fileRange);
		}
		if (arguments.size() != 6u) {
			ctx->Error(
			    "This intrinsic call requires 6 arguments to be provided. The first argument is the"
			    " vector value of the matrix to be stored. The second argument is the pointer to the"
			    " matrix data, the third argument is the stride which is the gap between the first element of"
			    " consecutive columns and is always greater than or equal to the number of rows. The fourth argument is a " +
			        ctx->color("bool") +
			        " value that determines the volatile nature of the memory access. The fifth"
			        " argument is the number of rows and the sixth argument is the number of columns",
			    fileRange);
		}
		auto vecVal = arguments[0]->emit(ctx);
		vecVal      = ir::Logic::handle_pass_semantics(ctx, vecVal->get_pass_type(), vecVal, arguments[0]->fileRange);
		if (not vecVal->get_ir_type()->is_same(firstTy)) {
			ctx->Error("The first argument is expected to be of type " + ctx->color(firstTy->to_string()) +
			               ", but got an expression of type " + ctx->color(vecVal->get_ir_type()->to_string()) +
			               " instead",
			           arguments[0]->fileRange);
		}
		auto ptrVal = arguments[1]->emit(ctx);
		ptrVal      = ir::Logic::handle_pass_semantics(ctx, ptrVal->get_pass_type(), ptrVal, arguments[1]->fileRange);
		auto voidPtrTy = ir::PtrType::get(false, ir::VoidType::get(ctx->irCtx->llctx), true,
		                                  ir::PtrOwner::of_anonymous(), false, ctx->irCtx);
		auto vecPtrTy  = ir::PtrType::get(false, firstTy, true, ir::PtrOwner::of_anonymous(), false, ctx->irCtx);
		if (not ptrVal->get_ir_type()->is_same(voidPtrTy) && not ptrVal->get_ir_type()->is_same(vecPtrTy)) {
			ctx->Error("The second argument is expected to be of type " + ctx->color(voidPtrTy->to_string()) + " or " +
			               ctx->color(vecPtrTy->to_string()) + ", but got an expression of type " +
			               ctx->color(ptrVal->get_ir_type()->to_string()) + " instead",
			           arguments[1]->fileRange);
		}
		auto strideVal = arguments[2]->emit(ctx);
		strideVal =
		    ir::Logic::handle_pass_semantics(ctx, strideVal->get_pass_type(), strideVal, arguments[2]->fileRange);
		if (not strideVal->get_ir_type()->is_same(ir::UnsignedType::create(64u, ctx->irCtx))) {
			ctx->Error("The third argument is expected to be of type " +
			               ctx->color(ir::UnsignedType::create(64u, ctx->irCtx)->to_string()) +
			               ", but got an expression of type " + ctx->color(strideVal->get_ir_type()->to_string()) +
			               " instead",
			           arguments[2]->fileRange);
		}
		auto volVal = arguments[3]->emit(ctx);
		volVal      = ir::Logic::handle_pass_semantics(ctx, volVal->get_pass_type(), volVal, arguments[3]->fileRange);
		if (not volVal->get_ir_type()->is_same(ir::UnsignedType::create_bool(ctx->irCtx))) {
			ctx->Error("The fourth argument is expected to be of type " +
			               ctx->color(ir::UnsignedType::create_bool(ctx->irCtx)->to_string()) +
			               " but got an expression of type " + ctx->color(volVal->get_ir_type()->to_string()) +
			               " instead",
			           arguments[3]->fileRange);
		}
		auto u32Ty  = ir::UnsignedType::create(32u, ctx->irCtx);
		auto rowVal = arguments[4]->emit(ctx);
		rowVal      = ir::Logic::handle_pass_semantics(ctx, rowVal->get_pass_type(), rowVal, arguments[4]->fileRange);
		if (not rowVal->get_ir_type()->is_same(u32Ty)) {
			ctx->Error("The fifth argument is expected to be of type " + ctx->color(u32Ty->to_string()) +
			               ", but got an expression of type " + ctx->color(rowVal->get_ir_type()->to_string()) +
			               " instead",
			           arguments[4]->fileRange);
		}
		auto colVal = arguments[5]->emit(ctx);
		colVal      = ir::Logic::handle_pass_semantics(ctx, colVal->get_pass_type(), colVal, arguments[5]->fileRange);
		if (not colVal->get_ir_type()->is_same(u32Ty)) {
			ctx->Error("The sixth argument is expected to be of type " + ctx->color(u32Ty->to_string()) +
			               ", but got an expression of type " + ctx->color(colVal->get_ir_type()->to_string()) +
			               " instead",
			           arguments[5]->fileRange);
		}
		if (rowVal->is_prerun_value() && strideVal->is_prerun_value()) {
			if (not llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldCompareInstruction(
			                                          llvm::CmpInst::ICMP_UGE, strideVal->get_llvm_constant(),
			                                          llvm::ConstantFoldCastInstruction(
			                                              llvm::ICmpInst::CastOps::ZExt, rowVal->get_llvm_constant(),
			                                              strideVal->get_ir_type()->get_llvm_type())))
			            ->getValue()
			            .getBoolValue()) {
				ctx->Error("The third argument is the stride which is expected to be"
				           " greater than or equal to the number of rows of the matrix",
				           arguments[1]->fileRange);
			}
		}
		if (rowVal->is_prerun_value() && colVal->is_prerun_value()) {
			auto mulRes = llvm::ConstantExpr::getMul(rowVal->get_llvm_constant(), colVal->get_llvm_constant());
			if (not llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldCompareInstruction(
			                                          llvm::CmpInst::ICMP_EQ,
			                                          llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx->irCtx->llctx),
			                                                                 firstTy->as_vector()->get_count()),
			                                          mulRes))
			            ->getValue()
			            .getBoolValue()) {
				ctx->Error(
				    "The vector type provided is " + ctx->color(firstTy->to_string()) +
				        " but the product of the number of rows and columns is " +
				        ctx->color(std::to_string(
				            *llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldConstant(mulRes, ctx->irCtx->dataLayout))
				                 ->getValue()
				                 .getRawData())) +
				        ". The element count does not match",
				    fileRange);
			}
		}
		ctx->mod->set_matrix_intrinsic_used();
		auto intrFn = llvm::Intrinsic::getOrInsertDeclaration(
		    ctx->mod->get_llvm_module(), llvm::Intrinsic::matrix_column_major_store, {firstTy->get_llvm_type()});
		return ir::Value::get(
		    ctx->irCtx->builder.CreateCall(intrFn->getFunctionType(), intrFn,
		                                   {vecVal->get_llvm(), ptrVal->get_llvm(), strideVal->get_llvm(),
		                                    volVal->get_llvm(), rowVal->get_llvm(), colVal->get_llvm()}),
		    ir::VoidType::get(ctx->irCtx->llctx), true);
	} else if (nmVal == IntrinsicID::read_cycle_counter) {
		if (not genArgs.empty()) {
			ctx->Error("This intrinsic does not require any generic parameters to be provided after the intrinsic ID",
			           fileRange);
		}
		if (not arguments.empty()) {
			ctx->Error("This intrinsic call does not require any arguments to be provided", fileRange);
		}
		auto intrFn =
		    llvm::Intrinsic::getOrInsertDeclaration(ctx->mod->get_llvm_module(), llvm::Intrinsic::readcyclecounter, {});
		return ir::Value::get(ctx->irCtx->builder.CreateCall(intrFn->getFunctionType(), intrFn, {}),
		                      ir::UnsignedType::create(64u, ctx->irCtx), true);
	} else if (nmVal == IntrinsicID::read_steady_counter) {
		if (not genArgs.empty()) {
			ctx->Error("This intrinsic does not require any generic parameters to be provided after the intrinsic ID",
			           fileRange);
		}
		if (not arguments.empty()) {
			ctx->Error("This intrinsic call does not require any arguments to be provided", fileRange);
		}
		auto intrFn = llvm::Intrinsic::getOrInsertDeclaration(ctx->mod->get_llvm_module(),
		                                                      llvm::Intrinsic::readsteadycounter, {});
		return ir::Value::get(ctx->irCtx->builder.CreateCall(intrFn->getFunctionType(), intrFn, {}),
		                      ir::UnsignedType::create(64u, ctx->irCtx), true);
	} else if (nmVal == IntrinsicID::give_address) {
		if (not genArgs.empty()) {
			ctx->Error("This intrinsic does not require any generic parameters to be provided after the Intrinsic ID",
			           fileRange);
		}
		if (not arguments.empty()) {
			ctx->Error("This intrinsic call does not require any arguments to be provided", fileRange);
		}
		if (not ctx->has_fn()) {
			ctx->Error("This intrinsic can only be used within a function", fileRange);
		}
		if (ctx->fn->get_ir_type()->as_function()->get_return_type()->get_type()->is_void()) {
			ctx->irCtx->Warning("The current function has a given type of " + ctx->color("void") +
			                        ", so this intrinsic will always return a " + ctx->color("null") + " pointer",
			                    fileRange);
		}
		auto intrFn =
		    llvm::Intrinsic::getOrInsertDeclaration(ctx->mod->get_llvm_module(), llvm::Intrinsic::returnaddress, {});
		return ir::Value::get(
		    ctx->irCtx->builder.CreateCall(intrFn->getFunctionType(), intrFn,
		                                   {llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx->irCtx->llctx), 0u)}),
		    ir::PtrType::get(true, ir::VoidType::get(ctx->irCtx->llctx), false, ir::PtrOwner::of_anonymous(), false,
		                     ctx->irCtx),
		    false);
	} else if (nmVal == IntrinsicID::caller_give_address) {
		if (not genArgs.empty()) {
			ctx->Error("This intrinsic does not require any generic parameters to be provided after the Intrinsic ID",
			           fileRange);
		}
		if (not arguments.empty()) {
			ctx->Error("This intrinsic call does not require any arguments to be provided", fileRange);
		}
		if (not ctx->has_fn()) {
			ctx->Error("This intrinsic can only be used within a function", fileRange);
		}
		auto intrFn =
		    llvm::Intrinsic::getOrInsertDeclaration(ctx->mod->get_llvm_module(), llvm::Intrinsic::returnaddress, {});
		return ir::Value::get(
		    ctx->irCtx->builder.CreateCall(intrFn->getFunctionType(), intrFn,
		                                   {llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx->irCtx->llctx), 1u)}),
		    ir::PtrType::get(true, ir::VoidType::get(ctx->irCtx->llctx), false, ir::PtrOwner::of_anonymous(), false,
		                     ctx->irCtx),
		    false);
	} else {
		ctx->Error("Unknown intrinsic found here", name->fileRange);
	}
	std::unreachable();
}

Json MetaIntrinsic::to_json() const {
	Vec<JsonValue> genJSON;
	for (auto arg : genArgs) {
		genJSON.push_back(arg->to_json());
	}
	Vec<JsonValue> argsJSON;
	for (auto arg : arguments) {
		argsJSON.push_back(arg->to_json());
	}
	return Json()
	    ._("nodeType", "metaIntrinsic")
	    ._("intrinsicName", name->to_json())
	    ._("genericArguments", genJSON)
	    ._("arguments", argsJSON)
	    ._("fileRange", fileRange);
}

} // namespace qat::ast
