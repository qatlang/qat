#include "./registers.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/void.hpp"

#include <llvm/IR/Intrinsics.h>

namespace qat::ast {

ir::Value* MetaRegisterRead::emit(EmitCtx* ctx) {
	auto nameExp = registerName->emit(ctx);
	if (not nameExp->get_ir_type()->is_text()) {
		ctx->Error("Expected an expression of type " + ctx->color(ir::TextType::get(ctx->irCtx, false)->to_string()) +
		               ", to represent the name of the register to read",
		           registerName->fileRange);
	}
	auto typeExp = registerType->emit(ctx);
	if (not typeExp->get_ir_type()->is_typed()) {
		ctx->Error("Expected a type here, but got an expression of type " +
		               ctx->color(typeExp->get_ir_type()->to_string()) + " instead",
		           registerType->fileRange);
	}
	auto regType = ir::TypeInfo::get_for(typeExp->get_llvm_constant())->type;
	if (not regType->is_unsigned()) {
		ctx->Error("Expected an unsigned integer type to be provided here, but got " +
		               ctx->color(regType->to_string()) + " instead",
		           registerType->fileRange);
	}
	auto regName = ir::TextType::value_to_string(nameExp);
	auto funcRes = llvm::Intrinsic::getOrInsertDeclaration(ctx->mod->get_llvm_module(),
	                                                       isVolatile ? llvm::Intrinsic::read_volatile_register
	                                                                  : llvm::Intrinsic::read_register,
	                                                       {regType->get_llvm_type()});
	return ir::Value::get(
	           ctx->irCtx->builder.CreateCall(
	               funcRes->getFunctionType(), funcRes,
	               {llvm::MetadataAsValue::get(
	                   ctx->irCtx->llctx,
	                   llvm::MDNode::get(ctx->irCtx->llctx, {llvm::MDString::get(ctx->irCtx->llctx, regName)}))}),
	           regType, true)
	    ->with_range(fileRange);
}

ir::Value* MetaRegisterWrite::emit(EmitCtx* ctx) {
	auto nameExp = registerName->emit(ctx);
	if (not nameExp->get_ir_type()->is_text()) {
		ctx->Error("Expected an expression of type " + ctx->color(ir::TextType::get(ctx->irCtx, false)->to_string()) +
		               ", to represent the name of the register to read",
		           registerName->fileRange);
	}
	auto typeExp = registerType->emit(ctx);
	if (not typeExp->get_ir_type()->is_typed()) {
		ctx->Error("Expected a type here, but got an expression of type " +
		               ctx->color(typeExp->get_ir_type()->to_string()) + " instead",
		           registerType->fileRange);
	}
	auto regType = ir::TypeInfo::get_for(typeExp->get_llvm_constant())->type;
	if (not regType->is_unsigned()) {
		ctx->Error("Expected an unsigned integer type to be provided here, but got " +
		               ctx->color(regType->to_string()) + " instead",
		           registerType->fileRange);
	}
	auto regName = ir::TextType::value_to_string(nameExp);
	auto funcRes = llvm::Intrinsic::getOrInsertDeclaration(ctx->mod->get_llvm_module(), llvm::Intrinsic::write_register,
	                                                       {regType->get_llvm_type()});
	auto valRes  = value->emit(ctx);
	if (not valRes->get_pass_type()->is_same(regType)) {
		ctx->Error("The provided expression is of type " + ctx->color(valRes->get_ir_type()->to_string()) +
		               " which does not match the type of the register provided, which is " +
		               ctx->color(regType->to_string()),
		           value->fileRange);
	}
	valRes = ir::Logic::handle_pass_semantics(ctx, valRes->get_pass_type(), valRes, value->fileRange);
	return ir::Value::get(
	           ctx->irCtx->builder.CreateCall(
	               funcRes->getFunctionType(), funcRes,
	               {llvm::MetadataAsValue::get(
	                    ctx->irCtx->llctx,
	                    llvm::MDNode::get(ctx->irCtx->llctx, {llvm::MDString::get(ctx->irCtx->llctx, regName)})),
	                valRes->get_llvm()}),
	           ir::VoidType::get(ctx->irCtx->llctx), true)
	    ->with_range(fileRange);
}

} // namespace qat::ast
