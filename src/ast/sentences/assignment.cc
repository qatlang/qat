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
	    (lhsVal->get_ir_type()->is_reference() && lhsVal->get_ir_type()->as_reference()->isSubtypeVariable())) {
		SHOW("Is variable nature")
		if (lhsVal->get_ir_type()->is_reference() || lhsVal->is_ghost_reference()) {
			if (lhsVal->is_reference()) {
				lhsVal->load_ghost_reference(ctx->irCtx->builder);
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
			    (lhsTy->is_reference() && lhsTy->as_reference()->get_subtype()->is_same(
			                                  expTy->is_reference() ? expTy->as_reference()->get_subtype() : expTy)) ||
			    (expTy->is_reference() && expTy->as_reference()->get_subtype()->is_same(
			                                  lhsTy->is_reference() ? lhsTy->as_reference()->get_subtype() : lhsTy))) {
				SHOW("The general types are the same")
				if (lhsVal->is_ghost_reference() && lhsTy->is_reference()) {
					SHOW("LHS is implicit pointer")
					lhsVal->load_ghost_reference(ctx->irCtx->builder);
					SHOW("Loaded implicit pointer")
				}
				if (expTy->is_reference() || expVal->is_ghost_reference()) {
					if (expTy->is_reference()) {
						expVal->load_ghost_reference(ctx->irCtx->builder);
						SHOW("Expression for assignment is of type "
						     << expTy->as_reference()->get_subtype()->to_string())
						expTy = expTy->as_reference()->get_subtype();
					}
					if (expTy->is_trivially_copyable() || expTy->is_trivially_movable()) {
						auto prevRef = expVal->get_llvm();
						expVal =
						    ir::Value::get(ctx->irCtx->builder.CreateLoad(expTy->get_llvm_type(), expVal->get_llvm()),
						                   expVal->get_ir_type(), expVal->is_variable());
						if (!expTy->is_trivially_copyable()) {
							if (expTy->is_reference() && !expTy->as_reference()->isSubtypeVariable()) {
								ctx->Error(
								    "This expression is of type " + ctx->color(expTy->to_string()) +
								        " which is a reference without variability and hence cannot be trivially moved from",
								    value->fileRange);
							} else if (!expVal->is_variable()) {
								ctx->Error(
								    "This expression does not have variability and hence cannot be trivially moved from",
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
						               " which is not trivially copyable or trivially movable. Please use " +
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
		if (lhsVal->get_ir_type()->is_reference()) {
			ctx->Error(
			    "Left hand side of the assignment cannot be assigned to because the reference does not have variability",
			    lhs->fileRange);
		} else if (lhsVal->get_ir_type()->is_mark()) {
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
