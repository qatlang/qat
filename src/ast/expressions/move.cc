#include "./move.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/maybe.hpp"

namespace qat::ast {

ir::Value* Move::emit(EmitCtx* ctx) {
	FnAtEnd fnObj{[&] { createIn = nullptr; }};
	if (isExpSelf) {
		if (not ctx->get_fn()->is_method()) {
			ctx->Error("Cannot perform move on the parent instance as this is not a member function", fileRange);
		} else {
			auto memFn = (ir::Method*)ctx->get_fn();
			if (memFn->is_static_method()) {
				ctx->Error("Cannot perform move on the parent instance as this is a static function", fileRange);
			}
			if (memFn->isConstructor()) {
				ctx->Error("Cannot perform move on the parent instance as this is a constructor", fileRange);
			}
		}
	} else {
		if (exp->nodeType() == NodeType::SELF) {
			ctx->Error("Do not use this syntax for moving from the parent instance. Use " + ctx->color("''move") +
			               " instead",
			           fileRange);
		}
	}
	auto* expEmit = exp->emit(ctx);
	auto* expTy   = expEmit->get_ir_type();
	if (expEmit->is_ghost_ref() || expTy->is_ref()) {
		if (expTy->is_ref()) {
			expEmit->load_ghost_ref(ctx->irCtx->builder);
		}
		auto* candTy = expEmit->is_ref() ? expEmit->get_ir_type()->as_ref()->get_subtype() : expEmit->get_ir_type();
		if (not isAssignment) {
			if (candTy->is_move_constructible()) {
				bool shouldLoadValue = false;
				if (isLocalDecl()) {
					createIn = ctx->get_fn()->get_block()->new_local(irName->value, candTy, isVar, irName->range);
				}
				if (canCreateIn()) {
					if (not createIn->is_ref() && not createIn->is_ghost_ref()) {
						ctx->Error(
						    "Trying to optimise the move by creating in-place, but the containing type is not a reference",
						    fileRange);
					}
					auto expTy = createIn->is_ghost_ref() ? createIn->get_ir_type()
					                                      : createIn->get_ir_type()->as_ref()->get_subtype();
					if (not type_check_create_in(candTy)) {
						ctx->Error(
						    "Trying to optimise the move by creating in-place, but the expression type is " +
						        ctx->color(candTy->to_string()) +
						        " which does not match with the underlying type for in-place creation which is " +
						        ctx->color(expTy->to_string()),
						    fileRange);
					}
				} else {
					shouldLoadValue = true;
					createIn        = ir::Value::get(ir::Logic::newAlloca(ctx->get_fn(), None, candTy->get_llvm_type()),
					                                 candTy, true);
				}
				(void)candTy->move_construct_value(ctx->irCtx, createIn, expEmit, ctx->get_fn());
				if (expEmit->is_local_value()) {
					ctx->get_fn()->get_block()->add_moved_value(expEmit->get_local_id().value());
				}
				if (shouldLoadValue) {
					return ir::Value::get(ctx->irCtx->builder.CreateLoad(candTy->get_llvm_type(), createIn->get_llvm()),
					                      candTy, true)
					    ->with_range(fileRange);
				} else {
					return get_creation_result(ctx->irCtx, candTy, fileRange);
				}
			} else if (candTy->has_simple_move()) {
				if (isLocalDecl()) {
					createIn = ctx->get_fn()->get_block()->new_local(irName->value, candTy, isVar, irName->range);
				}
				if (canCreateIn()) {
					if (not createIn->is_ref() && not createIn->is_ghost_ref()) {
						ctx->Error(
						    "Trying to optimise the move by creating in-place, but the containing type is not a reference",
						    fileRange);
					}
					auto expTy = createIn->is_ghost_ref() ? createIn->get_ir_type()
					                                      : createIn->get_ir_type()->as_ref()->get_subtype();
					if (not type_check_create_in(candTy)) {
						ctx->Error(
						    "Trying to optimise the move by creating in-place, but the expression type is " +
						        ctx->color(candTy->to_string()) +
						        " which does not match with the underlying type for in-place creation which is " +
						        ctx->color(expTy->to_string()),
						    fileRange);
					}
					ctx->irCtx->builder.CreateStore(
					    ctx->irCtx->builder.CreateLoad(candTy->get_llvm_type(), expEmit->get_llvm()),
					    createIn->get_llvm());
					ctx->irCtx->builder.CreateStore(llvm::Constant::getNullValue(candTy->get_llvm_type()),
					                                expEmit->get_llvm());
					if (expEmit->is_local_value()) {
						ctx->get_fn()->get_block()->add_moved_value(expEmit->get_local_id().value());
					}
					return get_creation_result(ctx->irCtx, candTy, fileRange);
				} else {
					auto* loadRes = ctx->irCtx->builder.CreateLoad(candTy->get_llvm_type(), expEmit->get_llvm());
					ctx->irCtx->builder.CreateStore(llvm::Constant::getNullValue(candTy->get_llvm_type()),
					                                expEmit->get_llvm());
					return ir::Value::get(loadRes, candTy, true)->with_range(fileRange);
				}
			} else {
				ctx->Error("Type " + ctx->color(candTy->to_string()) +
				               " does not have a move constructor and does not have simple-move",
				           fileRange);
			}
		} else {
			if (createIn->is_ghost_ref()
			        ? createIn->get_ir_type()->is_same(candTy)
			        : (createIn->is_ref() && createIn->get_ir_type()->as_ref()->get_subtype()->is_same(candTy))) {
				if (expEmit->is_ref()) {
					expEmit->load_ghost_ref(ctx->irCtx->builder);
				}
				if (candTy->has_simple_move()) {
					ctx->irCtx->builder.CreateStore(
					    ctx->irCtx->builder.CreateLoad(candTy->get_llvm_type(), expEmit->get_llvm()),
					    createIn->get_llvm());
					ctx->irCtx->builder.CreateStore(llvm::Constant::getNullValue(candTy->get_llvm_type()),
					                                expEmit->get_llvm());
					if (expEmit->is_local_value()) {
						ctx->get_fn()->get_block()->add_moved_value(expEmit->get_local_id().value());
					}
					return createIn;
				} else if (candTy->is_move_assignable()) {
					(void)candTy->move_assign_value(ctx->irCtx, createIn, expEmit, ctx->get_fn());
					if (expEmit->is_local_value()) {
						ctx->get_fn()->get_block()->add_moved_value(expEmit->get_local_id().value());
					}
					return createIn;
				} else {
					ctx->Error("Type " + ctx->color(candTy->to_string()) +
					               " does not have a move assignment operator and does not have simple-move",
					           fileRange);
				}
			} else {
				ctx->Error("Tried to optimise move assignment by in-place moving. The left hand side is of type " +
				               ctx->color(createIn->get_ir_type()->to_string()) +
				               " but the right hand side is of type " +
				               ctx->color(expEmit->get_ir_type()->to_string()) +
				               " and hence move assignment cannot be performed",
				           fileRange);
			}
		}
	} else {
		ctx->Error(
		    "This expression is a value. Expression provided should either be a reference, a local or a global variable. The provided value does not reside in an address and hence cannot be moved",
		    fileRange);
	}
	return nullptr;
}

Json Move::to_json() const {
	return Json()._("nodeType", "move")._("expression", exp->to_json())._("fileRange", fileRange);
}

} // namespace qat::ast
