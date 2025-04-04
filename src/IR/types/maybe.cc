#include "./maybe.hpp"
#include "../context.hpp"
#include "../control_flow.hpp"
#include "../value.hpp"
#include "./array.hpp"
#include "reference.hpp"

#include <llvm/Analysis/ConstantFolding.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>

namespace qat::ir {

MaybeType::MaybeType(Type* _subType, bool _isPacked, ir::Ctx* irCtx) : subTy(_subType), isPacked(_isPacked) {
	linkingName = "qat'maybe:[" + String(isPacked ? "pack," : "") + subTy->get_name_for_linking() + "]";
	// TODO - Error/warn if subtype is opaque
	llvmType = llvm::StructType::create(
	    irCtx->llctx,
	    {llvm::Type::getInt1Ty(irCtx->llctx),
	     has_sized_sub_type(irCtx) ? subTy->get_llvm_type() : llvm::Type::getInt8Ty(irCtx->llctx)},
	    linkingName, false);
}

MaybeType* MaybeType::get(Type* subTy, bool isPacked, ir::Ctx* irCtx) {
	for (auto* typ : allTypes) {
		if (typ->is_maybe()) {
			if (typ->as_maybe()->get_subtype()->is_same(subTy)) {
				return typ->as_maybe();
			}
		}
	}
	return std::construct_at(OwnNormal(MaybeType), subTy, isPacked, irCtx);
}

bool MaybeType::is_type_sized() const { return true; }

bool MaybeType::is_type_packed() const { return isPacked; }

bool MaybeType::has_sized_sub_type(ir::Ctx* irCtx) const { return subTy->is_type_sized(); }

Type* MaybeType::get_subtype() const { return subTy; }

String MaybeType::to_string() const { return "maybe:[" + String(isPacked ? "pack, " : "") + subTy->to_string() + "]"; }

bool MaybeType::is_copy_constructible() const { return has_simple_copy() || subTy->is_copy_constructible(); }

void MaybeType::copy_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) {
	if (has_simple_copy()) {
		irCtx->builder.CreateStore(irCtx->builder.CreateLoad(get_llvm_type(), second->get_llvm()), first->get_llvm());
	} else {
		if (is_copy_constructible()) {
			auto* tag =
			    irCtx->builder.CreateLoad(llvm::Type::getInt1Ty(irCtx->llctx),
			                              irCtx->builder.CreateStructGEP(get_llvm_type(), second->get_llvm(), 0u));
			auto* trueBlock  = ir::Block::create(fun, fun->get_block());
			auto* falseBlock = ir::Block::create(fun, fun->get_block());
			auto* restBlock  = ir::Block::create(fun, fun->get_block()->get_parent());
			restBlock->link_previous_block(fun->get_block());
			irCtx->builder.CreateCondBr(
			    irCtx->builder.CreateICmpEQ(tag, llvm::ConstantInt::getTrue(llvm::Type::getInt1Ty(irCtx->llctx))),
			    trueBlock->get_bb(), falseBlock->get_bb());
			trueBlock->set_active(irCtx->builder);
			subTy->copy_construct_value(
			    irCtx,
			    ir::Value::get(irCtx->builder.CreateStructGEP(get_llvm_type(), first->get_llvm(), 1u),
			                   ir::RefType::get(true, subTy, irCtx), false),
			    ir::Value::get(irCtx->builder.CreateStructGEP(get_llvm_type(), second->get_llvm(), 1u),
			                   ir::RefType::get(false, subTy, irCtx), false),
			    fun);
			irCtx->builder.CreateStore(llvm::ConstantInt::getTrue(llvm::Type::getInt1Ty(irCtx->llctx)),
			                           irCtx->builder.CreateStructGEP(get_llvm_type(), first->get_llvm(), 0u));
			(void)ir::add_branch(irCtx->builder, restBlock->get_bb());
			falseBlock->set_active(irCtx->builder);
			irCtx->builder.CreateStore(llvm::Constant::getNullValue(get_llvm_type()), first->get_llvm());
			(void)ir::add_branch(irCtx->builder, restBlock->get_bb());
			restBlock->set_active(irCtx->builder);
		} else {
			irCtx->Error("Could not copy construct an instance of type " + irCtx->color(to_string()), None);
		}
	}
}

bool MaybeType::is_copy_assignable() const { return has_simple_copy() || subTy->is_copy_assignable(); }

void MaybeType::copy_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) {
	if (has_simple_copy()) {
		irCtx->builder.CreateStore(irCtx->builder.CreateLoad(get_llvm_type(), second->get_llvm()), first->get_llvm());
	} else {
		if (is_copy_assignable()) {
			auto* firstTag =
			    irCtx->builder.CreateLoad(llvm::Type::getInt1Ty(irCtx->llctx),
			                              irCtx->builder.CreateStructGEP(get_llvm_type(), first->get_llvm(), 0u));
			auto* secondTag =
			    irCtx->builder.CreateLoad(llvm::Type::getInt1Ty(irCtx->llctx),
			                              irCtx->builder.CreateStructGEP(get_llvm_type(), second->get_llvm(), 0u));
			auto* tagMatchTrueBlock  = ir::Block::create(fun, fun->get_block());
			auto* tagMatchFalseBlock = ir::Block::create(fun, fun->get_block());
			auto* restBlock          = ir::Block::create(fun, fun->get_block()->get_parent());
			restBlock->link_previous_block(fun->get_block());
			irCtx->builder.CreateCondBr(irCtx->builder.CreateICmpEQ(firstTag, secondTag), tagMatchTrueBlock->get_bb(),
			                            tagMatchFalseBlock->get_bb());
			tagMatchTrueBlock->set_active(irCtx->builder);
			auto* tagTrueBlock  = ir::Block::create(fun, tagMatchTrueBlock);
			auto* tagFalseBlock = ir::Block::create(fun, tagMatchTrueBlock);
			irCtx->builder.CreateCondBr(
			    irCtx->builder.CreateICmpEQ(firstTag, llvm::ConstantInt::getTrue(llvm::Type::getInt1Ty(irCtx->llctx))),
			    tagTrueBlock->get_bb(), tagFalseBlock->get_bb());
			tagTrueBlock->set_active(irCtx->builder);
			subTy->copy_assign_value(
			    irCtx,
			    ir::Value::get(irCtx->builder.CreateStructGEP(get_llvm_type(), first->get_llvm(), 1u),
			                   ir::RefType::get(true, subTy, irCtx), false),
			    ir::Value::get(irCtx->builder.CreateStructGEP(get_llvm_type(), second->get_llvm(), 1u),
			                   ir::RefType::get(false, subTy, irCtx), false),
			    fun);
			(void)ir::add_branch(irCtx->builder, restBlock->get_bb());
			tagFalseBlock->set_active(irCtx->builder);
			(void)ir::add_branch(irCtx->builder, restBlock->get_bb());
			tagMatchFalseBlock->set_active(irCtx->builder);
			auto* firstValueBlock  = ir::Block::create(fun, tagMatchFalseBlock);
			auto* secondValueBlock = ir::Block::create(fun, tagMatchFalseBlock);
			irCtx->builder.CreateCondBr(
			    irCtx->builder.CreateICmpEQ(firstTag, llvm::ConstantInt::getTrue(llvm::Type::getInt1Ty(irCtx->llctx))),
			    firstValueBlock->get_bb(), secondValueBlock->get_bb());
			firstValueBlock->set_active(irCtx->builder);
			subTy->destroy_value(irCtx,
			                     ir::Value::get(irCtx->builder.CreateStructGEP(get_llvm_type(), first->get_llvm(), 1u),
			                                    ir::RefType::get(true, subTy, irCtx), false),
			                     fun);
			irCtx->builder.CreateStore(llvm::ConstantInt::getFalse(llvm::Type::getInt1Ty(irCtx->llctx)),
			                           irCtx->builder.CreateStructGEP(get_llvm_type(), first->get_llvm(), 0u));
			(void)ir::add_branch(irCtx->builder, restBlock->get_bb());
			secondValueBlock->set_active(irCtx->builder);
			subTy->copy_construct_value(
			    irCtx,
			    ir::Value::get(irCtx->builder.CreateStructGEP(get_llvm_type(), first->get_llvm(), 1u),
			                   ir::RefType::get(true, subTy, irCtx), false),
			    ir::Value::get(irCtx->builder.CreateStructGEP(get_llvm_type(), second->get_llvm(), 1u),
			                   ir::RefType::get(false, subTy, irCtx), false),
			    fun);
			irCtx->builder.CreateStore(llvm::ConstantInt::getTrue(llvm::Type::getInt1Ty(irCtx->llctx)),
			                           irCtx->builder.CreateStructGEP(get_llvm_type(), first->get_llvm(), 0u));
			(void)ir::add_branch(irCtx->builder, restBlock->get_bb());
			restBlock->set_active(irCtx->builder);
		} else {
			irCtx->Error("Could not copy assign an instance of type " + irCtx->color(to_string()), None);
		}
	}
}

bool MaybeType::is_move_constructible() const { return has_simple_move() || subTy->is_move_constructible(); }

void MaybeType::move_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) {
	if (has_simple_copy()) {
		irCtx->builder.CreateStore(irCtx->builder.CreateLoad(get_llvm_type(), second->get_llvm()), first->get_llvm());
		irCtx->builder.CreateStore(llvm::Constant::getNullValue(get_llvm_type()), second->get_llvm());
	} else {
		if (is_move_constructible()) {
			auto* tag =
			    irCtx->builder.CreateLoad(llvm::Type::getInt1Ty(irCtx->llctx),
			                              irCtx->builder.CreateStructGEP(get_llvm_type(), second->get_llvm(), 0u));
			auto* trueBlock  = ir::Block::create(fun, fun->get_block());
			auto* falseBlock = ir::Block::create(fun, fun->get_block());
			auto* restBlock  = ir::Block::create(fun, fun->get_block()->get_parent());
			restBlock->link_previous_block(fun->get_block());
			irCtx->builder.CreateCondBr(
			    irCtx->builder.CreateICmpEQ(tag, llvm::ConstantInt::getTrue(llvm::Type::getInt1Ty(irCtx->llctx))),
			    trueBlock->get_bb(), falseBlock->get_bb());
			trueBlock->set_active(irCtx->builder);
			subTy->move_construct_value(
			    irCtx,
			    ir::Value::get(irCtx->builder.CreateStructGEP(get_llvm_type(), first->get_llvm(), 1u),
			                   ir::RefType::get(true, subTy, irCtx), false),
			    ir::Value::get(irCtx->builder.CreateStructGEP(get_llvm_type(), second->get_llvm(), 1u),
			                   ir::RefType::get(false, subTy, irCtx), false),
			    fun);
			irCtx->builder.CreateStore(llvm::ConstantInt::getTrue(llvm::Type::getInt1Ty(irCtx->llctx)),
			                           irCtx->builder.CreateStructGEP(get_llvm_type(), first->get_llvm(), 0u));
			irCtx->builder.CreateStore(llvm::ConstantInt::getFalse(llvm::Type::getInt1Ty(irCtx->llctx)),
			                           irCtx->builder.CreateStructGEP(get_llvm_type(), second->get_llvm(), 0u));
			(void)ir::add_branch(irCtx->builder, restBlock->get_bb());
			falseBlock->set_active(irCtx->builder);
			irCtx->builder.CreateStore(llvm::Constant::getNullValue(get_llvm_type()), first->get_llvm());
			(void)ir::add_branch(irCtx->builder, restBlock->get_bb());
			restBlock->set_active(irCtx->builder);
		} else {
			irCtx->Error("Could not move construct an instance of type " + irCtx->color(to_string()), None);
		}
	}
}

bool MaybeType::is_move_assignable() const { return has_simple_move() || subTy->is_move_assignable(); }

void MaybeType::move_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) {
	if (has_simple_move()) {
		irCtx->builder.CreateStore(irCtx->builder.CreateLoad(get_llvm_type(), second->get_llvm()), first->get_llvm());
		irCtx->builder.CreateStore(llvm::Constant::getNullValue(get_llvm_type()), second->get_llvm());
	} else {
		if (is_move_assignable()) {
			auto* firstTag =
			    irCtx->builder.CreateLoad(llvm::Type::getInt1Ty(irCtx->llctx),
			                              irCtx->builder.CreateStructGEP(get_llvm_type(), first->get_llvm(), 0u));
			auto* secondTag =
			    irCtx->builder.CreateLoad(llvm::Type::getInt1Ty(irCtx->llctx),
			                              irCtx->builder.CreateStructGEP(get_llvm_type(), second->get_llvm(), 0u));
			auto* tagMatchTrueBlock  = ir::Block::create(fun, fun->get_block());
			auto* tagMatchFalseBlock = ir::Block::create(fun, fun->get_block());
			auto* restBlock          = ir::Block::create(fun, fun->get_block()->get_parent());
			restBlock->link_previous_block(fun->get_block());
			irCtx->builder.CreateCondBr(irCtx->builder.CreateICmpEQ(firstTag, secondTag), tagMatchTrueBlock->get_bb(),
			                            tagMatchFalseBlock->get_bb());
			tagMatchTrueBlock->set_active(irCtx->builder);
			auto* tagTrueBlock  = ir::Block::create(fun, tagMatchTrueBlock);
			auto* tagFalseBlock = ir::Block::create(fun, tagMatchTrueBlock);
			irCtx->builder.CreateCondBr(
			    irCtx->builder.CreateICmpEQ(firstTag, llvm::ConstantInt::getTrue(llvm::Type::getInt1Ty(irCtx->llctx))),
			    tagTrueBlock->get_bb(), tagFalseBlock->get_bb());
			tagTrueBlock->set_active(irCtx->builder);
			subTy->move_assign_value(
			    irCtx,
			    ir::Value::get(irCtx->builder.CreateStructGEP(get_llvm_type(), first->get_llvm(), 1u),
			                   ir::RefType::get(true, subTy, irCtx), false),
			    ir::Value::get(irCtx->builder.CreateStructGEP(get_llvm_type(), second->get_llvm(), 1u),
			                   ir::RefType::get(false, subTy, irCtx), false),
			    fun);
			irCtx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt1Ty(irCtx->llctx), 0u, false),
			                           irCtx->builder.CreateStructGEP(get_llvm_type(), second->get_llvm(), 0u));
			(void)ir::add_branch(irCtx->builder, restBlock->get_bb());
			tagFalseBlock->set_active(irCtx->builder);
			(void)ir::add_branch(irCtx->builder, restBlock->get_bb());
			tagMatchFalseBlock->set_active(irCtx->builder);
			auto* firstValueBlock  = ir::Block::create(fun, tagMatchFalseBlock);
			auto* secondValueBlock = ir::Block::create(fun, tagMatchFalseBlock);
			irCtx->builder.CreateCondBr(
			    irCtx->builder.CreateICmpEQ(firstTag, llvm::ConstantInt::getTrue(llvm::Type::getInt1Ty(irCtx->llctx))),
			    firstValueBlock->get_bb(), secondValueBlock->get_bb());
			firstValueBlock->set_active(irCtx->builder);
			subTy->destroy_value(irCtx,
			                     ir::Value::get(irCtx->builder.CreateStructGEP(get_llvm_type(), first->get_llvm(), 1u),
			                                    ir::RefType::get(true, subTy, irCtx), false),
			                     fun);
			irCtx->builder.CreateStore(llvm::ConstantInt::getFalse(llvm::Type::getInt1Ty(irCtx->llctx)),
			                           irCtx->builder.CreateStructGEP(get_llvm_type(), first->get_llvm(), 0u));
			(void)ir::add_branch(irCtx->builder, restBlock->get_bb());
			secondValueBlock->set_active(irCtx->builder);
			subTy->move_construct_value(
			    irCtx,
			    ir::Value::get(irCtx->builder.CreateStructGEP(get_llvm_type(), first->get_llvm(), 1u),
			                   ir::RefType::get(true, subTy, irCtx), false),
			    ir::Value::get(irCtx->builder.CreateStructGEP(get_llvm_type(), second->get_llvm(), 1u),
			                   ir::RefType::get(false, subTy, irCtx), false),
			    fun);
			irCtx->builder.CreateStore(llvm::ConstantInt::getTrue(llvm::Type::getInt1Ty(irCtx->llctx)),
			                           irCtx->builder.CreateStructGEP(get_llvm_type(), first->get_llvm(), 0u));
			(void)ir::add_branch(irCtx->builder, restBlock->get_bb());
			restBlock->set_active(irCtx->builder);
		} else {
			irCtx->Error("Could not move assign an instance of type " + irCtx->color(to_string()), None);
		}
	}
}

bool MaybeType::is_destructible() const { return subTy->is_destructible(); }

void MaybeType::destroy_value(ir::Ctx* irCtx, ir::Value* instance, ir::Function* fun) {
	if (is_destructible()) {
		if (instance->is_ref()) {
			instance->load_ghost_ref(irCtx->builder);
		}
		auto* inst      = instance->get_llvm();
		auto* currBlock = fun->get_block();
		auto* trueBlock = ir::Block::create(fun, currBlock);
		auto* restBlock = ir::Block::create(fun, currBlock->get_parent());
		restBlock->link_previous_block(currBlock);
		irCtx->builder.CreateCondBr(irCtx->builder.CreateLoad(llvm::Type::getInt1Ty(irCtx->llctx),
		                                                      irCtx->builder.CreateStructGEP(llvmType, inst, 0u)),
		                            trueBlock->get_bb(), restBlock->get_bb());
		trueBlock->set_active(irCtx->builder);
		subTy->destroy_value(irCtx,
		                     ir::Value::get(irCtx->builder.CreateStructGEP(llvmType, inst, 1u),
		                                    ir::RefType::get(false, subTy, irCtx), false),
		                     fun);
		(void)ir::add_branch(irCtx->builder, restBlock->get_bb());
		restBlock->set_active(irCtx->builder);
	} else {
		irCtx->Error("Could not destroy an instance of type " + irCtx->color(to_string()), None);
	}
}

Maybe<String> MaybeType::to_prerun_generic_string(ir::PrerunValue* value) const {
	if (can_be_prerun_generic()) {
		if (llvm::cast<llvm::ConstantInt>(value->get_llvm_constant()->getAggregateElement(0u))
		        ->getValue()
		        .getBoolValue()) {
			SHOW("Maybe has value")
			return "is(" +
			       subTy
			           ->to_prerun_generic_string(
			               ir::PrerunValue::get(value->get_llvm_constant()->getAggregateElement(1u), subTy))
			           .value() +
			       ")";
		} else {
			return "none";
		}
	} else {
		return None;
	}
}

Maybe<bool> MaybeType::equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const {
	if (first->get_ir_type()->is_same(second->get_ir_type())) {
		const bool firstHasValue = llvm::cast<llvm::ConstantInt>(first->get_llvm_constant()->getAggregateElement(0u))
		                               ->getValue()
		                               .getBoolValue();
		const bool secondHasValue = llvm::cast<llvm::ConstantInt>(second->get_llvm_constant()->getAggregateElement(0u))
		                                ->getValue()
		                                .getBoolValue();
		if (firstHasValue == secondHasValue) {
			if (firstHasValue) {
				return subTy->equality_of(
				    irCtx, ir::PrerunValue::get(first->get_llvm_constant()->getAggregateElement(1u), subTy),
				    ir::PrerunValue::get(
				        llvm::ConstantFoldConstant(
				            llvm::ConstantExpr::getBitCast(second->get_llvm_constant()->getAggregateElement(1u),
				                                           subTy->get_llvm_type()),
				            irCtx->dataLayout),
				        subTy));
			} else {
				return true;
			}
		} else {
			return false;
		}
	} else {
		return None;
	}
}

} // namespace qat::ir
