#include "./copy_move_pointer.hpp"
#include "../../IR/control_flow.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/native_type.hpp"
#include "../../IR/types/pointer.hpp"
#include "../../IR/types/void.hpp"

namespace qat::ast {

ir::Value* CopyMovePointer::emit(EmitCtx* ctx) {
	auto target = candidate->emit(ctx);
	target      = ir::Logic::handle_pass_semantics(ctx, target->get_pass_type(), target, candidate->fileRange);
	if (not target->get_ir_type()->is_ptr()) {
		ctx->Error("Expected a " + String(kind == CopyMovePtrKind::PTR ? "pointer" : "multi-pointer") +
		               " value here, but found an expression of type " +
		               ctx->color(target->get_ir_type()->to_string()) + " here",
		           fileRange);
		std::unreachable();
	}
	auto source = arguments[0]->emit(ctx);
	source      = ir::Logic::handle_pass_semantics(ctx, source->get_pass_type(), source, arguments[0]->fileRange);
	const auto PTR_NAME      = String(kind == CopyMovePtrKind::PTR ? "pointer" : "multi-pointer");
	const auto OPERATION     = String(isMove ? "move" : "copy");
	const auto OP_PAST_TENSE = String(isMove ? "moved" : "copied");
	if (not source->get_ir_type()->is_ptr()) {
		ctx->Error("Expected a " + PTR_NAME + " value here, but found an expression of type " +
		               ctx->color(source->get_ir_type()->to_string()) + " instead for the source " + PTR_NAME +
		               " of this operation",
		           arguments[0]->fileRange);
		std::unreachable();
	}
	auto ptrTy = target->get_ir_type()->as_ptr();
	if (not ptrTy->get_locality().is_heap()) {
		ctx->Error(OPERATION + "-constructing a " + PTR_NAME +
		               " from another one are only supported if they have the " + ctx->color("heap") +
		               " locality, but got a pointer value of type " + ctx->color(ptrTy->to_string()) + " instead",
		           fileRange);
		std::unreachable();
	}
	if (ptrTy->is_nullable()) {
		ctx->Error("Nullable " + String(kind == CopyMovePtrKind::PTR ? "pointers" : "multi-pointers") +
		               " cannot be used in this operation. This is because there are two " +
		               (kind == CopyMovePtrKind::PTR ? "pointers" : "multi-pointers") +
		               " involved in this operation, and either one of them being " + ctx->color("null") +
		               " can lead to confusing and uintuitive behaviour",
		           fileRange);
		std::unreachable();
	}
	if (not ptrTy->is_same(source->get_ir_type())) {
		ctx->Error("For " + OPERATION + "-constructing elements in a " + PTR_NAME + " from another " + PTR_NAME +
		               ","
		               " both types are expected to be non-nullable, to have the same value-type and also have the " +
		               ctx->color("heap") + "locality. The type of the multi-pointer that is being moved from is " +
		               ctx->color(ptrTy->to_string()) + ", but the expression provided for the source " + PTR_NAME +
		               " of this operation is of type " + ctx->color(source->get_ir_type()->to_string()),
		           fileRange);
		std::unreachable();
	}
	auto subTy = ptrTy->get_subtype();
	if (isMove and not(subTy->has_simple_move()) and not(subTy->is_move_constructible())) {
		ctx->Error(
		    "The value-type of the pointer type " + ctx->color(ptrTy->to_string()) + " is " +
		        ctx->color(subTy->to_string()) +
		        " which does not have simple-move and is not move-constructible. Non-movable types do not require this operation."
		        " You can use this operation conditionally by checking whether the value-type"
		        " is movable, using the prerun condition " +
		        ctx->color(String(ptrTy->is_multi() ? "multiPointerExpression" : "pointerExpression") +
		                   "'type'value_type()'has_move()") +
		        " or you can specifically check whether the value-type is move-constructible, using the prerun condition " +
		        ctx->color(String(ptrTy->is_multi() ? "multiPointerExpression" : "pointerExpression") +
		                   "'type'value_type()'is_move_constructible()"),
		    fileRange);
		std::unreachable();
	} else if (not(isMove) and not(subTy->has_simple_copy()) and not(subTy->is_copy_constructible())) {
		ctx->Error(
		    "The value-type of the pointer type " + ctx->color(ptrTy->to_string()) + " is " +
		        ctx->color(subTy->to_string()) +
		        " which does not have simple-copy and is not copy-constructible. Non-copyable types do not require this operation."
		        " You can use this operation conditionally by checking whether the value-type"
		        " is copyable, using the prerun condition " +
		        ctx->color(String(ptrTy->is_multi() ? "multiPointerExpression" : "pointerExpression") +
		                   "'type'value_type()'has_copy()") +
		        " or you can specifically check whether the value-type is copy-constructible, using the prerun condition " +
		        ctx->color(String(ptrTy->is_multi() ? "multiPointerExpression" : "pointerExpression") +
		                   "'type'value_type()'is_copy_constructible()"),
		    fileRange);
		std::unreachable();
	}
	if (ptrTy->is_multi()) {
		if (kind == CopyMovePtrKind::PTR) {
			ctx->Error(
			    "Invalid syntax for " + OPERATION + "-constructing elements in the multi-pointer type " +
			        ctx->color(ptrTy->to_string()) + ". This syntax is meant for normal pointers only. Use " +
			        ctx->color("targetMultiPointer'" + OPERATION + ":multi(sourceMultiPointer)") + ", " +
			        ctx->color("targetMultiPointer'" + OPERATION + ":from(sourceMultiPointer, firstIndexInclusive)") +
			        ", " +
			        ctx->color("targetMultiPointer'" + OPERATION + ":to(sourceMultiPointer, lastIndexExclusive)") +
			        " or " +
			        ctx->color("targetMultiPointer'" + OPERATION +
			                   ":in(sourceMultiPointer, firstIndex, lastIndexExclusive)") +
			        " for multi-pointers",
			    fileRange);
			std::unreachable();
		}
		const auto index     = ctx->get_fn()->get_str_comparison_index(ctx->irCtx);
		const auto indexTy   = index->get_ir_type()->get_llvm_type();
		const auto sourcePtr = ctx->irCtx->builder.CreateExtractValue(source->get_llvm(), {0u});
		const auto sourceLen = ctx->irCtx->builder.CreateExtractValue(source->get_llvm(), {1u});
		const auto targetPtr = ctx->irCtx->builder.CreateExtractValue(target->get_llvm(), {0u});
		const auto targetLen = ctx->irCtx->builder.CreateExtractValue(target->get_llvm(), {1u});

		if (kind == CopyMovePtrKind::FROM or kind == CopyMovePtrKind::MULTI) {
			const auto currBlock    = ctx->get_fn()->get_block();
			const auto lenFailBlock = ir::Block::create(ctx->get_fn(), currBlock);
			const auto restBlock    = ir::Block::create(ctx->get_fn(), currBlock->get_parent());
			restBlock->link_previous_block(currBlock);
			ctx->irCtx->builder.CreateCondBr(ctx->irCtx->builder.CreateICmpUGT(sourceLen, targetLen),
			                                 lenFailBlock->get_bb(), restBlock->get_bb());
			lenFailBlock->set_active(ctx->irCtx->builder);
			ir::Logic::panic_in_function(
			    ctx->get_fn(),
			    {
			        ir::TextType::create_value(ctx->irCtx, ctx->mod,
			                                   "The length of the multi-pointer that is being " + OP_PAST_TENSE +
			                                       " from, is "),
			        ir::Value::get(sourceLen, index->get_ir_type(), true),
			        ir::TextType::create_value(
			            ctx->irCtx, ctx->mod,
			            ", which is not less than or equal to the length of the multi-pointer that is " + OPERATION +
			                "-constructed, which is "),
			        ir::Value::get(targetLen, index->get_ir_type(), true),
			        ir::TextType::create_value(ctx->irCtx, ctx->mod,
			                                   ". Cannot perform '" + OPERATION + ":" + kind_to_string(kind) +
			                                       " as it requires this condition to be true"),
			    },
			    {}, fileRange, ctx);
			(void)ir::add_branch(ctx->irCtx->builder, restBlock->get_bb());
		}
		ir::Value* fromInd = nullptr;
		if (kind == CopyMovePtrKind::MULTI) {
			ctx->irCtx->builder.CreateStore(llvm::ConstantInt::get(indexTy, 0u), index->get_llvm());
		} else if (kind == CopyMovePtrKind::FROM or kind == CopyMovePtrKind::RANGE) {
			fromInd = arguments[1]->emit(ctx);
			fromInd = ir::Logic::handle_pass_semantics(ctx, fromInd->get_pass_type(), fromInd, arguments[1]->fileRange);
			if (not(fromInd->get_ir_type()->is_native_type() and
			        fromInd->get_ir_type()->as_native_type()->is_native_usize())) {
				ctx->Error("Expected a value of type " + ctx->color("usize") +
				               " here for the inclusive starting index, but got an expression of type " +
				               ctx->color(fromInd->get_ir_type()->to_string()) + " instead",
				           arguments[1]->fileRange);
				std::unreachable();
			}
			ctx->irCtx->builder.CreateStore(fromInd->get_llvm(), index->get_llvm());
			const auto currBlock    = ctx->get_fn()->get_block();
			const auto indFailBlock = ir::Block::create(ctx->get_fn(), currBlock);
			const auto restBlock    = ir::Block::create(ctx->get_fn(), currBlock->get_parent());
			restBlock->link_previous_block(currBlock);
			ctx->irCtx->builder.CreateCondBr(
			    ctx->irCtx->builder.CreateOr(ctx->irCtx->builder.CreateICmpUGE(fromInd->get_llvm(), sourceLen),
			                                 ctx->irCtx->builder.CreateICmpUGE(fromInd->get_llvm(), targetLen)),
			    indFailBlock->get_bb(), restBlock->get_bb());
			indFailBlock->set_active(ctx->irCtx->builder);
			ir::Logic::panic_in_function(
			    ctx->get_fn(),
			    {
			        ir::TextType::create_value(ctx->irCtx, ctx->mod,
			                                   "The inclusive starting index given in '" + OPERATION + ":" +
			                                       kind_to_string(kind) + " to " + OPERATION +
			                                       " objects in the multi-pointer is "),
			        fromInd,
			        ir::TextType::create_value(ctx->irCtx, ctx->mod,
			                                   ", which should be less than the length of both multi-pointers."
			                                   " The length of the multi-pointer that is being " +
			                                       OP_PAST_TENSE + " from is "),
			        ir::Value::get(sourceLen, ir::NativeType::get_usize(ctx->irCtx), true),
			        ir::TextType::create_value(ctx->irCtx, ctx->mod,
			                                   ", and the length of the multi-pointer that is being " + OPERATION +
			                                       "-constructed is "),
			        ir::Value::get(targetLen, ir::NativeType::get_usize(ctx->irCtx), true),
			    },
			    {}, arguments[1]->fileRange, ctx);
			restBlock->set_active(ctx->irCtx->builder);
		}
		// NOTE - DO NOT MERGE THE BELOW CONDITION TO THE ABOVE if-else AS BOTH HANDLES THE 'copy:in and 'move:in CASE
		ir::Value* toInd = nullptr;
		if (kind == CopyMovePtrKind::TO or kind == CopyMovePtrKind::RANGE) {
			auto relArg = kind == CopyMovePtrKind::TO ? arguments[1] : arguments[2];
			toInd       = relArg->emit(ctx);
			toInd       = ir::Logic::handle_pass_semantics(ctx, toInd->get_pass_type(), toInd, relArg->fileRange);
			if (not(toInd->get_ir_type()->is_native_type()) and
			    not(toInd->get_ir_type()->as_native_type()->is_native_usize())) {
				ctx->Error("Expected a value of type " + ctx->color("usize") +
				               " here for the exclusive end index, but got an expression of type " +
				               ctx->color(toInd->get_ir_type()->to_string()) + " instead",
				           relArg->fileRange);
				std::unreachable();
			}
			const auto currBlock    = ctx->get_fn()->get_block();
			const auto indFailBlock = ir::Block::create(ctx->get_fn(), currBlock);
			const auto restBlock    = ir::Block::create(ctx->get_fn(), currBlock->get_parent());
			restBlock->link_previous_block(currBlock);
			ctx->irCtx->builder.CreateCondBr(
			    ctx->irCtx->builder.CreateOr(ctx->irCtx->builder.CreateICmpUGT(toInd->get_llvm(), sourceLen),
			                                 ctx->irCtx->builder.CreateICmpUGT(toInd->get_llvm(), targetLen)),
			    indFailBlock->get_bb(), restBlock->get_bb());
			indFailBlock->set_active(ctx->irCtx->builder);
			ir::Logic::panic_in_function(
			    ctx->get_fn(),
			    {
			        ir::TextType::create_value(ctx->irCtx, ctx->mod,
			                                   "The exclusive ending index given in '" + OPERATION + ":" +
			                                       kind_to_string(kind) + " to " + OPERATION +
			                                       " objects in the multi-pointer is "),
			        toInd,
			        ir::TextType::create_value(
			            ctx->irCtx, ctx->mod,
			            ", which should be less than or equal to the length of both multi-pointers."
			            " The length of the multi-pointer that is being " +
			                OP_PAST_TENSE + " from is "),
			        ir::Value::get(sourceLen, ir::NativeType::get_usize(ctx->irCtx), true),
			        ir::TextType::create_value(ctx->irCtx, ctx->mod,
			                                   ", and the length of the multi-pointer that is being " + OPERATION +
			                                       "-constructed is "),
			        ir::Value::get(targetLen, ir::NativeType::get_usize(ctx->irCtx), true),
			    },
			    {}, relArg->fileRange, ctx);
			restBlock->set_active(ctx->irCtx->builder);
		} else {
			toInd = ir::Value::get(sourceLen, ir::NativeType::get_usize(ctx->irCtx), true);
		}
		if (isMove ? subTy->has_simple_move() : subTy->has_simple_copy()) {
			if (isMove and subTy->has_simple_copy()) {
				ctx->irCtx->Warning(
				    "The value-type of the " + PTR_NAME + " type is " + ctx->color(subTy->to_string()) +
				        " which has simple-copy and simple-move. Simple-copy requires less steps and is more performant"
				        " than simple-move. It is always recommended to use simple-copy over simple-move unless you"
				        " explicitly need to zero-initialize the data in the source multi-pointer",
				    fileRange);
			}
			const auto subTyStoreSize = llvm::ConstantInt::get(
			    sourceLen->getType(), (u64)ctx->irCtx->dataLayout.getTypeStoreSize(subTy->get_llvm_type()));
			switch (kind) {
				case CopyMovePtrKind::MULTI: {
					const auto sourceByteSize = ctx->irCtx->builder.CreateMul(sourceLen, subTyStoreSize);
					ctx->irCtx->builder.CreateIntrinsic(
					    llvm::Intrinsic::memcpy_inline,
					    {targetPtr->getType(), sourcePtr->getType(), sourceLen->getType()},
					    {targetPtr, sourcePtr, sourceByteSize, llvm::ConstantInt::getFalse(ctx->irCtx->llctx)});
					if (isMove) {
						ctx->irCtx->builder.CreateIntrinsic(
						    llvm::Intrinsic::memset_inline, {sourcePtr->getType(), sourceLen->getType()},
						    {sourcePtr, llvm::ConstantInt::get(llvm::Type::getInt8Ty(ctx->irCtx->llctx), 0u),
						     sourceByteSize});
					}
					break;
				}
				case CopyMovePtrKind::FROM: {
					const auto sourceByteSize = ctx->irCtx->builder.CreateMul(
					    ctx->irCtx->builder.CreateSub(sourceLen, fromInd->get_llvm()), subTyStoreSize);
					const auto sourceOffset =
					    ctx->irCtx->builder.CreateInBoundsGEP(subTy->get_llvm_type(), sourcePtr, {fromInd->get_llvm()});
					ctx->irCtx->builder.CreateIntrinsic(
					    llvm::Intrinsic::memcpy_inline,
					    {targetPtr->getType(), sourcePtr->getType(), sourceLen->getType()},
					    {ctx->irCtx->builder.CreateInBoundsGEP(subTy->get_llvm_type(), targetPtr,
					                                           {fromInd->get_llvm()}),
					     sourceOffset, sourceByteSize});
					if (isMove) {
						ctx->irCtx->builder.CreateIntrinsic(
						    llvm::Intrinsic::memset_inline, {sourcePtr->getType(), sourceLen->getType()},
						    {sourceOffset, llvm::ConstantInt::get(llvm::Type::getInt8Ty(ctx->irCtx->llctx), 0u),
						     sourceByteSize});
					}
					break;
				}
				case CopyMovePtrKind::TO: {
					const auto sourceByteSize = ctx->irCtx->builder.CreateMul(toInd->get_llvm(), subTyStoreSize);
					ctx->irCtx->builder.CreateIntrinsic(
					    llvm::Intrinsic::memcpy_inline,
					    {targetPtr->getType(), sourcePtr->getType(), sourceLen->getType()},
					    {targetPtr, sourcePtr, sourceByteSize});
					if (isMove) {
						ctx->irCtx->builder.CreateIntrinsic(
						    llvm::Intrinsic::memset_inline, {sourcePtr->getType(), sourceLen->getType()},
						    {sourcePtr, llvm::ConstantInt::get(llvm::Type::getInt8Ty(ctx->irCtx->llctx), 0u),
						     sourceByteSize});
					}
					break;
				}
				case CopyMovePtrKind::RANGE: {
					const auto sourceByteSize = ctx->irCtx->builder.CreateMul(
					    ctx->irCtx->builder.CreateSub(toInd->get_llvm(), fromInd->get_llvm()), subTyStoreSize);
					const auto sourceOffset =
					    ctx->irCtx->builder.CreateInBoundsGEP(subTy->get_llvm_type(), sourcePtr, {fromInd->get_llvm()});
					ctx->irCtx->builder.CreateIntrinsic(
					    llvm::Intrinsic::memcpy_inline,
					    {targetPtr->getType(), sourcePtr->getType(), sourceLen->getType()},
					    {ctx->irCtx->builder.CreateInBoundsGEP(subTy->get_llvm_type(), targetPtr,
					                                           {fromInd->get_llvm()}),
					     sourceOffset, sourceByteSize});
					if (isMove) {
						ctx->irCtx->builder.CreateIntrinsic(
						    llvm::Intrinsic::memset_inline, {sourcePtr->getType(), sourceLen->getType()},
						    {sourceOffset, llvm::ConstantInt::get(llvm::Type::getInt8Ty(ctx->irCtx->llctx), 0u),
						     sourceByteSize});
					}
					break;
				}
				default:
					break;
			}
		} else {
			const auto currBlock = ctx->get_fn()->get_block();
			const auto mainBlock = ir::Block::create(ctx->get_fn(), currBlock);
			ir::Block* restBlock = ir::Block::create(ctx->get_fn(), currBlock->get_parent());
			restBlock->link_previous_block(currBlock);
			const auto startIndex      = (kind == CopyMovePtrKind::MULTI or kind == CopyMovePtrKind::TO)
			                                 ? llvm::cast<llvm::Value>(llvm::ConstantInt::get(indexTy, 0u, false))
			                                 : ctx->irCtx->builder.CreateLoad(indexTy, index->get_llvm());
			auto       lenCheckInitial = ctx->irCtx->builder.CreateICmpULT(startIndex, toInd->get_llvm());
			ctx->irCtx->builder.CreateCondBr(lenCheckInitial, mainBlock->get_bb(), restBlock->get_bb());
			mainBlock->set_active(ctx->irCtx->builder);
			if (isMove) {
				const auto elemRefTy =
				    ir::RefType::get(true, ptrTy->get_subtype(), ptrTy->get_address_space(), ctx->irCtx);
				subTy->move_construct_value(
				    ctx->irCtx,
				    ir::Value::get(ctx->irCtx->builder.CreateInBoundsGEP(
				                       subTy->get_llvm_type(), targetPtr,
				                       {ctx->irCtx->builder.CreateLoad(indexTy, index->get_llvm())}),
				                   elemRefTy, false),
				    ir::Value::get(ctx->irCtx->builder.CreateInBoundsGEP(
				                       subTy->get_llvm_type(), sourcePtr,
				                       {ctx->irCtx->builder.CreateLoad(indexTy, index->get_llvm())}),
				                   elemRefTy, false),
				    ctx->get_fn());
			} else {
				const auto sourceElemPtr = ctx->irCtx->builder.CreateInBoundsGEP(
				    subTy->get_llvm_type(), sourcePtr, {ctx->irCtx->builder.CreateLoad(indexTy, index->get_llvm())});
				const auto elemRefTy =
				    ir::RefType::get(true, ptrTy->get_subtype(), ptrTy->get_address_space(), ctx->irCtx);
				subTy->copy_construct_value(
				    ctx->irCtx,
				    ir::Value::get(ctx->irCtx->builder.CreateInBoundsGEP(
				                       subTy->get_llvm_type(), targetPtr,
				                       {ctx->irCtx->builder.CreateLoad(indexTy, index->get_llvm())}),
				                   elemRefTy, false),
				    ir::Value::get(sourceElemPtr, elemRefTy, false), ctx->get_fn());
				if (shouldDestroyAfter) {
					if (subTy->is_destructible()) {
						subTy->destroy_value(ctx->irCtx, ir::Value::get(sourceElemPtr, elemRefTy, false),
						                     ctx->get_fn());
					}
				}
			}
			ctx->irCtx->builder.CreateStore(
			    ctx->irCtx->builder.CreateAdd(ctx->irCtx->builder.CreateLoad(indexTy, index->get_llvm()),
			                                  llvm::ConstantInt::get(indexTy, 1u, false)),
			    index->get_llvm());
			auto indexLoad = ctx->irCtx->builder.CreateLoad(indexTy, index->get_llvm());
			ctx->irCtx->builder.CreateCondBr(ctx->irCtx->builder.CreateICmpULT(indexLoad, toInd->get_llvm()),
			                                 mainBlock->get_bb(), restBlock->get_bb());
			restBlock->set_active(ctx->irCtx->builder);
		}
	} else {
		if (kind != CopyMovePtrKind::PTR) {
			ctx->Error("Invalid syntax for " + OPERATION + "-constructing value in the pointer type " +
			               ctx->color(ptrTy->to_string()) + ". This syntax is only supported for multi-pointers. Use " +
			               ctx->color("'" + OPERATION + ":ptr()") + " instead",
			           fileRange);
			std::unreachable();
		}
		if (isMove ? subTy->has_simple_move() : subTy->has_simple_copy()) {
			if (isMove and subTy->has_simple_copy()) {
				ctx->irCtx->Warning(
				    "The value-type of the pointer type is " + ctx->color(subTy->to_string()) +
				        " which has simple-copy and simple-move. Simple-copy requires less steps and is more performant"
				        " than simple-move. It is always recommended to use simple-copy over simple-move unless you"
				        " explicitly need to zero-initialize the data in the source pointer",
				    fileRange);
			}
			const auto uint32Ty        = llvm::Type::getInt32Ty(ctx->irCtx->llctx);
			const auto subStoreSizeVal = (usize)ctx->irCtx->dataLayout.getTypeStoreSize(subTy->get_llvm_type());
			const auto subStoreSize    = llvm::ConstantInt::get(uint32Ty, subStoreSizeVal);
			if (subStoreSizeVal > 128) {
				ctx->irCtx->builder.CreateIntrinsic(
				    llvm::Intrinsic::memcpy_inline,
				    {target->get_llvm()->getType(), source->get_llvm()->getType(), uint32Ty},
				    {target->get_llvm(), source->get_llvm(), subStoreSize});
				if (isMove) {
					ctx->irCtx->builder.CreateIntrinsic(
					    llvm::Intrinsic::memset_inline, {source->get_llvm()->getType(), uint32Ty},
					    {source->get_llvm(), llvm::ConstantInt::get(llvm::Type::getInt8Ty(ctx->irCtx->llctx), 0u),
					     subStoreSize});
				}
			} else {
				ctx->irCtx->builder.CreateStore(
				    ctx->irCtx->builder.CreateLoad(subTy->get_llvm_type(), source->get_llvm()), target->get_llvm());
				ctx->irCtx->builder.CreateStore(llvm::Constant::getNullValue(subTy->get_llvm_type()),
				                                source->get_llvm());
			}
		} else {
			const auto elemRefTy = ir::RefType::get(true, ptrTy->get_subtype(), ptrTy->get_address_space(), ctx->irCtx);
			if (isMove) {
				subTy->move_construct_value(
				    ctx->irCtx,
				    ir::Value::get(target->get_llvm(),
				                   ir::RefType::get(true, ptrTy->get_subtype(), ptrTy->get_address_space(), ctx->irCtx),
				                   false),
				    ir::Value::get(
				        source->get_llvm(),
				        ir::RefType::get(false, ptrTy->get_subtype(), ptrTy->get_address_space(), ctx->irCtx), false),
				    ctx->get_fn());
			} else {
				subTy->copy_construct_value(ctx->irCtx, ir::Value::get(target->get_llvm(), elemRefTy, false),
				                            ir::Value::get(source->get_llvm(), elemRefTy, false), ctx->get_fn());
				if (shouldDestroyAfter) {
					if (subTy->is_destructible()) {
						subTy->destroy_value(ctx->irCtx, ir::Value::get(source->get_llvm(), elemRefTy, false),
						                     ctx->get_fn());
					}
				}
			}
		}
	}
	return ir::Value::get(llvm::UndefValue::get(llvm::Type::getVoidTy(ctx->irCtx->llctx)),
	                      ir::VoidType::get(ctx->irCtx->llctx), false);
}

} // namespace qat::ast