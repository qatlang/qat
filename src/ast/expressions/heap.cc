#include "./heap.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/void.hpp"

#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>

namespace qat::ast {

#define MALLOC_ARG_BITWIDTH 64

ir::Value* HeapGet::emit(EmitCtx* ctx) {
	auto       mod      = ctx->mod;
	ir::Value* countRes = nullptr;
	if (count) {
		if (count->nodeType() == NodeType::DEFAULT) {
			ctx->Error(
			    "Default value for usize is 0, which is an invalid value for the number of instances to allocate",
			    count->fileRange);
		} else if (count->has_type_inferrance()) {
			count->as_type_inferrable()->set_inference_type(ir::NativeType::get_usize(ctx->irCtx));
		}
		countRes = count->emit(ctx);
		countRes->load_ghost_ref(ctx->irCtx->builder);
		if (countRes->get_ir_type()->is_ref()) {
			countRes = ir::Value::get(
			    ctx->irCtx->builder.CreateLoad(countRes->get_ir_type()->as_ref()->get_subtype()->get_llvm_type(),
			                                   countRes->get_llvm()),
			    countRes->get_ir_type()->as_ref()->get_subtype(), false);
		}
		if (not countRes->get_ir_type()->is_native_type() ||
		    not countRes->get_ir_type()->as_native_type()->is_usize()) {
			ctx->Error("The number of instances to allocate should be of " +
			               ir::NativeType::get_usize(ctx->irCtx)->to_string() + " type",
			           count->fileRange);
		}
	}
	// FIXME - Check for zero values
	llvm::Value* size   = nullptr;
	auto*        typRes = type->emit(ctx);
	if (typRes->is_void()) {
		size = llvm::ConstantInt::get(
		    llvm::Type::getInt64Ty(ctx->irCtx->llctx),
		    mod->get_llvm_module()->getDataLayout().getTypeAllocSize(llvm::Type::getInt8Ty(ctx->irCtx->llctx)));
	} else {
		size =
		    llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->irCtx->llctx),
		                           mod->get_llvm_module()->getDataLayout().getTypeAllocSize(typRes->get_llvm_type()));
	}
	auto  mallocName = mod->link_internal_dependency(ir::InternalDependency::malloc, ctx->irCtx, fileRange);
	auto* resTy      = ir::MarkType::get(true, typRes, false, ir::MarkOwner::of_heap(), count != nullptr, ctx->irCtx);
	auto* mallocFn   = mod->get_llvm_module()->getFunction(mallocName);
	if (resTy->is_slice()) {
		SHOW("Creating alloca for multi pointer")
		auto* llAlloca = ir::Logic::newAlloca(ctx->get_fn(), None, resTy->get_llvm_type());
		ctx->irCtx->builder.CreateStore(
		    ctx->irCtx->builder.CreatePointerCast(
		        ctx->irCtx->builder.CreateCall(
		            mallocFn->getFunctionType(), mallocFn,
		            {count ? ctx->irCtx->builder.CreateMul(size, countRes->get_llvm()) : size}),
		        llvm::PointerType::get(resTy->get_subtype()->get_llvm_type(),
		                               ctx->irCtx->dataLayout.getProgramAddressSpace())),
		    ctx->irCtx->builder.CreateStructGEP(resTy->get_llvm_type(), llAlloca, 0u));
		SHOW("Storing the size of the multi pointer")
		ctx->irCtx->builder.CreateStore(countRes->get_llvm(),
		                                ctx->irCtx->builder.CreateStructGEP(resTy->get_llvm_type(), llAlloca, 1u));
		return ir::Value::get(llAlloca, resTy, false);
	} else {
		return ir::Value::get(ctx->irCtx->builder.CreatePointerCast(
		                          ctx->irCtx->builder.CreateCall(
		                              mallocFn->getFunctionType(), mallocFn,
		                              {count ? ctx->irCtx->builder.CreateMul(size, countRes->get_llvm()) : size}),
		                          resTy->get_llvm_type()),
		                      resTy, false);
	}
}

Json HeapGet::to_json() const {
	return Json()
	    ._("nodeType", "heapGet")
	    ._("type", type->to_json())
	    ._("count", count->to_json())
	    ._("fileRange", fileRange);
}

ir::Value* HeapPut::emit(EmitCtx* ctx) {
	if (ptr->nodeType() == NodeType::NULL_MARK) {
		ctx->Error("Null mark cannot be freed", ptr->fileRange);
	}
	auto* exp   = ptr->emit(ctx);
	auto  expTy = exp->is_ref() ? exp->get_ir_type()->as_ref()->get_subtype() : exp->get_ir_type();
	if (expTy->is_mark()) {
		auto ptrTy = expTy->as_mark();
		if (not ptrTy->get_owner().is_of_heap()) {
			ctx->Error("The mark type of this expression is " + ctx->color(ptrTy->to_string()) +
			               " which does not have heap ownership and hence cannot be used here",
			           ptr->fileRange);
		}
	} else {
		ctx->Error(
		    "Expecting an expression having a pointer type with heap ownership. The provided expression is of type " +
		        ctx->color(expTy->to_string()) + ". Other expression types are not supported",
		    ptr->fileRange);
	}
	llvm::Value* candExp = nullptr;
	if (exp->get_ir_type()->is_ref() || exp->is_ghost_ref()) {
		if (exp->get_ir_type()->is_ref()) {
			exp->load_ghost_ref(ctx->irCtx->builder);
		}
		exp = ir::Value::get(ctx->irCtx->builder.CreateLoad(expTy->get_llvm_type(), exp->get_llvm()), expTy, false);
	}
	// FIXME - CONSIDER PRERUN POINTERS
	candExp =
	    expTy->as_mark()->is_slice() ? ctx->irCtx->builder.CreateExtractValue(exp->get_llvm(), {0u}) : exp->get_llvm();
	auto* mod      = ctx->mod;
	auto  freeName = mod->link_internal_dependency(ir::InternalDependency::free, ctx->irCtx, fileRange);
	auto* freeFn   = mod->get_llvm_module()->getFunction(freeName);
	ctx->irCtx->builder.CreateCall(
	    freeFn->getFunctionType(), freeFn,
	    {ctx->irCtx->builder.CreatePointerCast(candExp, llvm::Type::getInt8Ty(ctx->irCtx->llctx)->getPointerTo())});
	return ir::Value::get(nullptr, ir::VoidType::get(ctx->irCtx->llctx), false);
}

Json HeapPut::to_json() const {
	return Json()._("nodeType", "heapPut")._("pointer", ptr->to_json())._("fileRange", fileRange);
}

ir::Value* HeapGrow::emit(EmitCtx* ctx) {
	auto* typ    = type->emit(ctx);
	auto* ptrVal = ptr->emit(ctx);
	// FIXME - Add check to see if the new size is lower than the previous size
	ir::MarkType* ptrType = nullptr;
	if (ptrVal->is_ref()) {
		if (not ptrVal->get_ir_type()->as_ref()->has_variability()) {
			ctx->Error("This reference does not have variability and hence the pointer inside cannot be grown",
			           ptr->fileRange);
		}
		if (ptrVal->get_ir_type()->as_ref()->get_subtype()->is_mark()) {
			ptrType = ptrVal->get_ir_type()->as_ref()->get_subtype()->as_mark();
			ptrVal->load_ghost_ref(ctx->irCtx->builder);
			if (not ptrType->get_owner().is_of_heap()) {
				ctx->Error("The ownership of this pointer is not " + ctx->color("heap") +
				               " and hence cannot be used in heap:grow",
				           fileRange);
			}
			if (not ptrType->is_slice()) {
				ctx->Error("The type of the expression is " +
				               ctx->color(ptrVal->get_ir_type()->as_ref()->get_subtype()->to_string()) +
				               " which is not a multi pointer and hence cannot be grown",
				           ptr->fileRange);
			}
			if (not ptrType->get_subtype()->is_same(typ)) {
				ctx->Error("The first argument should be a pointer to " + ctx->color(typ->to_string()), ptr->fileRange);
			}
		} else {
			ctx->Error("The first argument should be a pointer to " + ctx->color(typ->to_string()), ptr->fileRange);
		}
	} else if (ptrVal->is_ghost_ref()) {
		if (ptrVal->get_ir_type()->is_mark()) {
			if (ptrVal->is_variable()) {
				ctx->Error("This expression is not a variable", fileRange);
			}
			ptrType = ptrVal->get_ir_type()->as_mark();
			if (not ptrType->get_owner().is_of_heap()) {
				ctx->Error("The ownership of this pointer is not " + ctx->color("heap") +
				               " and hence cannot be used in heap:grow",
				           fileRange);
			}
			if (not ptrType->is_slice()) {
				ctx->Error("The type of the expression is " + ctx->color(ptrVal->get_ir_type()->to_string()) +
				               " which is not a multi pointer and hence cannot be grown",
				           ptr->fileRange);
			}
			if (not ptrType->get_subtype()->is_same(typ)) {
				ctx->Error("The first argument to heap'grow should be a pointer to " + ctx->color(typ->to_string()),
				           ptr->fileRange);
			}
		} else {
			ctx->Error("The first argument to heap'grow should be a pointer to " + ctx->color(typ->to_string()),
			           ptr->fileRange);
		}
	} else {
		ptrType = ptrVal->get_ir_type()->as_mark();
		if (not ptrType->get_owner().is_of_heap()) {
			ctx->Error("Expected a multipointer with " + ctx->color("heap") +
			               " ownership. The ownership of this pointer is " +
			               ctx->color(ptrType->get_owner().to_string()) + " and hence cannot be used.",
			           fileRange);
		}
		if (not ptrType->is_slice()) {
			ctx->Error("The type of the expression is " + ctx->color(ptrVal->get_ir_type()->to_string()) +
			               " which is not a multi pointer and hence cannot be used here",
			           ptr->fileRange);
		}
		if (not ptrType->get_subtype()->is_same(typ)) {
			ctx->Error("The first argument should be a pointer to " + ctx->color(typ->to_string()), ptr->fileRange);
		}
	}
	auto* countVal = count->emit(ctx);
	countVal->load_ghost_ref(ctx->irCtx->builder);
	if (countVal->is_ref()) {
		countVal =
		    ir::Value::get(ctx->irCtx->builder.CreateLoad(
		                       countVal->get_ir_type()->as_ref()->get_subtype()->get_llvm_type(), countVal->get_llvm()),
		                   countVal->get_ir_type()->as_ref()->get_subtype(), false);
	}
	if (countVal->get_ir_type()->is_native_type() && countVal->get_ir_type()->as_native_type()->is_usize()) {
		auto  reallocName = ctx->mod->link_internal_dependency(ir::InternalDependency::realloc, ctx->irCtx, fileRange);
		auto* reallocFn   = ctx->mod->get_llvm_module()->getFunction(reallocName);
		auto* ptrRes      = ctx->irCtx->builder.CreatePointerCast(
            ctx->irCtx->builder.CreateCall(
                reallocFn->getFunctionType(), reallocFn,
                {ctx->irCtx->builder.CreatePointerCast(
                     ctx->irCtx->builder.CreateLoad(
                         llvm::PointerType::get(ptrType->get_subtype()->get_llvm_type(),
		                                             ctx->irCtx->dataLayout.getProgramAddressSpace()),
                         ptrVal->is_value()
		                          ? ctx->irCtx->builder.CreateExtractValue(ptrVal->get_llvm(), {0u})
		                          : ctx->irCtx->builder.CreateStructGEP(ptrType->get_llvm_type(), ptrVal->get_llvm(), 0u)),
                     llvm::Type::getInt8Ty(ctx->irCtx->llctx)->getPointerTo()),
		              ctx->irCtx->builder.CreateMul(
                     countVal->get_llvm(),
                     llvm::ConstantInt::get(
                         ir::NativeType::get_usize(ctx->irCtx)->get_llvm_type(),
                         ctx->irCtx->dataLayout.getTypeStoreSize(ptrType->get_subtype()->get_llvm_type())))}),
            llvm::PointerType::get(ptrType->as_mark()->get_subtype()->get_llvm_type(),
		                                ctx->irCtx->dataLayout.getProgramAddressSpace()));
		auto* resAlloc = ir::Logic::newAlloca(ctx->get_fn(), None, ptrType->get_llvm_type());
		SHOW("Storing raw pointer into multipointer")
		ctx->irCtx->builder.CreateStore(ptrRes,
		                                ctx->irCtx->builder.CreateStructGEP(ptrType->get_llvm_type(), resAlloc, 0u));
		SHOW("Storing count into multipointer")
		ctx->irCtx->builder.CreateStore(countVal->get_llvm(),
		                                ctx->irCtx->builder.CreateStructGEP(ptrType->get_llvm_type(), resAlloc, 1u));
		return ir::Value::get(resAlloc, ptrType, false);
	} else {
		ctx->Error("The number of units to reallocate should be of " +
		               ctx->color(ir::NativeType::get_usize(ctx->irCtx)->to_string()) + " type",
		           count->fileRange);
	}
}

Json HeapGrow::to_json() const {
	return Json()
	    ._("nodeType", "heapGrow")
	    ._("type", type->to_json())
	    ._("pointer", ptr->to_json())
	    ._("count", count->to_json())
	    ._("fileRange", fileRange);
}

} // namespace qat::ast
