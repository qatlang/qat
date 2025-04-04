#include "./await.hpp"
#include "../../IR/control_flow.hpp"
#include "../../IR/types/future.hpp"
#include "../../IR/types/void.hpp"

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>

namespace qat::ast {

ir::Value* Await::emit(EmitCtx* ctx) {
	SHOW("Exp nodetype is: " << (int)exp->nodeType())
	SHOW("Starting await emitting")
	auto* expEmit = exp->emit(ctx);
	if ((expEmit->is_ref() && expEmit->get_ir_type()->as_ref()->get_subtype()->is_future()) ||
	    (expEmit->is_ghost_ref() && expEmit->get_ir_type()->is_future())) {
		auto* futureTy = expEmit->is_ref() ? expEmit->get_ir_type()->as_ref()->get_subtype()->as_future()
		                                   : expEmit->get_ir_type()->as_future();
		if (expEmit->is_ref()) {
			expEmit->load_ghost_ref(ctx->irCtx->builder);
		}
		auto* fun        = ctx->get_fn();
		auto* trueBlock  = ir::Block::create(fun, fun->get_block());
		auto* falseBlock = ir::Block::create(fun, fun->get_block());
		auto* restBlock  = ir::Block::create(fun, nullptr);
		restBlock->link_previous_block(fun->get_block());
		ctx->irCtx->builder.CreateCondBr(
		    ctx->irCtx->builder.CreateLoad(
		        llvm::Type::getInt1Ty(ctx->irCtx->llctx),
		        ctx->irCtx->builder.CreatePointerCast(
		            ctx->irCtx->builder.CreateInBoundsGEP(
		                llvm::Type::getInt64Ty(ctx->irCtx->llctx),
		                ctx->irCtx->builder.CreateLoad(
		                    llvm::Type::getInt64Ty(ctx->irCtx->llctx)
		                        ->getPointerTo(ctx->irCtx->dataLayout.getProgramAddressSpace()),
		                    ctx->irCtx->builder.CreateStructGEP(futureTy->get_llvm_type(), expEmit->get_llvm(), 1u)),
		                {llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->irCtx->llctx), 1u)}),
		            llvm::Type::getInt1Ty(ctx->irCtx->llctx)
		                ->getPointerTo(ctx->irCtx->dataLayout.getProgramAddressSpace()))),
		    trueBlock->get_bb(), falseBlock->get_bb());
		trueBlock->set_active(ctx->irCtx->builder);
		(void)ir::add_branch(ctx->irCtx->builder, restBlock->get_bb());
		falseBlock->set_active(ctx->irCtx->builder);
		ctx->irCtx->builder.CreateCondBr(
		    ctx->irCtx->builder.CreateLoad(
		        llvm::Type::getInt1Ty(ctx->irCtx->llctx),
		        ctx->irCtx->builder.CreatePointerCast(
		            ctx->irCtx->builder.CreateInBoundsGEP(
		                llvm::Type::getInt64Ty(ctx->irCtx->llctx),
		                ctx->irCtx->builder.CreateLoad(
		                    llvm::Type::getInt64Ty(ctx->irCtx->llctx)
		                        ->getPointerTo(ctx->irCtx->dataLayout.getProgramAddressSpace()),
		                    ctx->irCtx->builder.CreateStructGEP(futureTy->get_llvm_type(), expEmit->get_llvm(), 1u)),
		                {llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->irCtx->llctx), 1u)}),
		            llvm::Type::getInt1Ty(ctx->irCtx->llctx)
		                ->getPointerTo(ctx->irCtx->dataLayout.getProgramAddressSpace()))),
		    restBlock->get_bb(), falseBlock->get_bb());
		(void)ir::add_branch(ctx->irCtx->builder, restBlock->get_bb());
		restBlock->set_active(ctx->irCtx->builder);
		if (futureTy->get_subtype()->is_void()) {
			return ir::Value::get(nullptr, ir::VoidType::get(ctx->irCtx->llctx), false)->with_range(fileRange);
		} else {
			return ir::Value::get(
			           ctx->irCtx->builder.CreatePointerCast(
			               ctx->irCtx->builder.CreateInBoundsGEP(
			                   llvm::Type::getInt8Ty(ctx->irCtx->llctx),
			                   ctx->irCtx->builder.CreatePointerCast(
			                       ctx->irCtx->builder.CreateInBoundsGEP(
			                           llvm::Type::getInt64Ty(ctx->irCtx->llctx),
			                           ctx->irCtx->builder.CreateLoad(
			                               llvm::Type::getInt64Ty(ctx->irCtx->llctx)
			                                   ->getPointerTo(ctx->irCtx->dataLayout.getProgramAddressSpace()),
			                               ctx->irCtx->builder.CreateStructGEP(futureTy->get_llvm_type(),
			                                                                   expEmit->get_llvm(), 1u)),
			                           {llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->irCtx->llctx), 1u)}),
			                       llvm::Type::getInt8Ty(ctx->irCtx->llctx)
			                           ->getPointerTo(ctx->irCtx->dataLayout.getProgramAddressSpace())),
			                   {llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->irCtx->llctx), 1u)}),
			               futureTy->get_subtype()->get_llvm_type()->getPointerTo(
			                   ctx->irCtx->dataLayout.getProgramAddressSpace())),
			           ir::RefType::get(expEmit->is_ref() ? expEmit->get_ir_type()->as_ref()->has_variability()
			                                              : expEmit->is_variable(),
			                            futureTy->get_subtype(), ctx->irCtx),
			           false)
			    ->with_range(fileRange);
		}
	} else {
		ctx->Error("The expression should be a " + ctx->color("future") + " to use await", exp->fileRange);
	}
	return nullptr;
}

Json Await::to_json() const {
	return Json()._("nodeType", "await")._("expression", exp->to_json())._("fileRange", fileRange);
}

} // namespace qat::ast
