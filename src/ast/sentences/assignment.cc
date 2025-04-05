#include "./assignment.hpp"
#include "../../IR/function.hpp"
#include "../expressions/copy.hpp"
#include "../expressions/move.hpp"

#include <llvm/IR/Constants.h>

namespace qat::ast {

#define MAX_RESPONSIVE_BITWIDTH 64u

ir::Value* Assignment::emit(EmitCtx* ctx) {
	auto* lhsVal = lhs->emit(ctx);
	if (value->has_type_inferrance()) {
		value->as_type_inferrable()->set_inference_type(lhsVal->get_ir_type());
	}
	SHOW("Emitted lhs of Assignment")
	if (lhsVal->is_variable() ||
	    (lhsVal->get_ir_type()->is_ref() && lhsVal->get_ir_type()->as_ref()->has_variability())) {
		SHOW("Is variable nature")
		if (lhsVal->get_ir_type()->is_ref() || lhsVal->is_ghost_ref()) {
			if (lhsVal->is_ref()) {
				lhsVal->load_ghost_ref(ctx->irCtx->builder);
			}
			if (value->nodeType() == NodeType::COPY) {
				auto copyExp          = (ast::Copy*)value;
				copyExp->isAssignment = true;
				copyExp->setCreateIn(lhsVal);
				(void)value->emit(ctx);
				copyExp->unsetCreateIn();
				return nullptr;
			} else if (value->nodeType() == NodeType::MOVE_EXPRESSION) {
				auto moveExp          = (ast::Move*)value;
				moveExp->isAssignment = true;
				moveExp->setCreateIn(lhsVal);
				(void)value->emit(ctx);
				moveExp->unsetCreateIn();
				return nullptr;
			}
			auto* expVal = value->emit(ctx);
			SHOW("Getting IR types")
			auto* lhsTy = lhsVal->get_ir_type();
			auto* expTy = expVal->get_ir_type();
			if (lhsTy->is_same(expTy) || lhsTy->isCompatible(expTy) ||
			    (lhsTy->is_ref() &&
			     lhsTy->as_ref()->get_subtype()->is_same(expTy->is_ref() ? expTy->as_ref()->get_subtype() : expTy)) ||
			    (expTy->is_ref() &&
			     expTy->as_ref()->get_subtype()->is_same(lhsTy->is_ref() ? lhsTy->as_ref()->get_subtype() : lhsTy))) {
				SHOW("The general types are the same")
				if (lhsVal->is_ghost_ref() && lhsTy->is_ref()) {
					SHOW("LHS is implicit pointer")
					lhsVal->load_ghost_ref(ctx->irCtx->builder);
					SHOW("Loaded implicit pointer")
				}
				if (expTy->is_ref() || expVal->is_ghost_ref()) {
					if (expTy->is_ref()) {
						expVal->load_ghost_ref(ctx->irCtx->builder);
						SHOW("Expression for assignment is of type " << expTy->as_ref()->get_subtype()->to_string())
						expTy = expTy->as_ref()->get_subtype();
					}
					if (expTy->has_simple_copy() || expTy->has_simple_move()) {
						auto prevRef = expVal->get_llvm();
						expVal =
						    ir::Value::get(ctx->irCtx->builder.CreateLoad(expTy->get_llvm_type(), expVal->get_llvm()),
						                   expVal->get_ir_type(), expVal->is_variable());
						if (not expTy->has_simple_copy()) {
							if (expTy->is_ref() && not expTy->as_ref()->has_variability()) {
								ctx->Error(
								    "This expression is of type " + ctx->color(expTy->to_string()) +
								        " which is a reference without variability and hence simple-move is not possible",
								    value->fileRange);
							} else if (not expVal->is_variable()) {
								ctx->Error(
								    "This expression does not have variability and hence simple-move is not possible",
								    fileRange);
							}
							// FIXME - Use zero/default value
							ctx->irCtx->builder.CreateStore(llvm::ConstantExpr::getNullValue(expTy->get_llvm_type()),
							                                prevRef);
						}
						SHOW("Creating store with LHS type: " << lhsVal->get_ir_type()->to_string() << " and RHS type: "
						                                      << expVal->get_ir_type()->to_string())
						ctx->irCtx->builder.CreateStore(expVal->get_llvm(), lhsVal->get_llvm());
					} else {
						ctx->Error("The expression on the right hand side is of type " +
						               ctx->color(expTy->to_string()) +
						               " which does not have simple-copy and simple-move. Please use " +
						               ctx->color("'copy") + " or " + ctx->color("'move") + " accordingly",
						           value->fileRange);
					}
				} else {
					ctx->irCtx->builder.CreateStore(expVal->get_llvm(), lhsVal->get_llvm());
				}
				return nullptr;
			} else {
				ctx->Error("Type of the left hand side of the assignment is " + ctx->color(lhsTy->to_string()) +
				               " and the type of right hand side is " + ctx->color(expTy->to_string()) +
				               ". The types of both sides are not compatible for assignment. Please check the logic",
				           fileRange);
			}
		} else {
			ctx->Error("Left hand side of the assignment cannot be assigned to as it is value", lhs->fileRange);
		}
	} else {
		if (lhsVal->get_ir_type()->is_ref()) {
			ctx->Error(
			    "Left hand side of the assignment cannot be assigned to because the reference does not have variability",
			    lhs->fileRange);
		} else if (lhsVal->get_ir_type()->is_ptr()) {
			ctx->Error(
			    "Left hand side of the assignment cannot be assigned to because it is of pointer type. If you intend to change the "
			    "value pointed to by this pointer, consider dereferencing it before assigning",
			    lhs->fileRange);
		} else {
			ctx->Error("Left hand side of the assignment cannot be assigned to because it does not have variability",
			           lhs->fileRange);
		}
	}
	return nullptr;
}

Json Assignment::to_json() const {
	return Json()
	    ._("nodeType", "assignment")
	    ._("lhs", lhs->to_json())
	    ._("rhs", value->to_json())
	    ._("fileRange", fileRange);
}

} // namespace qat::ast
