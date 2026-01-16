#include "./pointer.hpp"
#include "../../IR/types/pointer.hpp"
#include "../expression.hpp"

#include <llvm/Analysis/ConstantFolding.h>
#include <llvm/IR/DerivedTypes.h>

namespace qat::ast {

void PtrType::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) {
	type->update_dependencies(phase, ir::DependType::partial, ent, ctx);
	if (owner.candidate) {
		owner.candidate->update_dependencies(phase, ir::DependType::partial, ent, ctx);
	}
	if (addressSpace.has_value() && (addressSpace.value().value != nullptr)) {
		UPDATE_DEPS(addressSpace.value().value);
	}
}

Maybe<usize> PtrType::get_type_bitsize(EmitCtx* ctx) const {
	if (addressSpace.has_value()) {
		return None;
	}
	return (usize)(ctx->mod->get_llvm_module()->getDataLayout().getTypeAllocSizeInBits(
	    isMulti ? llvm::cast<llvm::Type>(
	                  llvm::StructType::create({llvm::PointerType::get(llvm::Type::getInt8Ty(ctx->irCtx->llctx),
	                                                                   ctx->irCtx->dataLayout.getProgramAddressSpace()),
	                                            llvm::Type::getInt64Ty(ctx->irCtx->llctx)}))
	            : llvm::cast<llvm::Type>(llvm::PointerType::get(llvm::Type::getInt8Ty(ctx->irCtx->llctx),
	                                                            ctx->irCtx->dataLayout.getProgramAddressSpace()))));
}

ir::Type* PtrType::emit(EmitCtx* ctx) {
	auto subTy = type->emit(ctx);
	if (isSubtypeVar && subTy->is_function()) {
		ctx->Error(
		    "The subtype of this pointer type is a function type, and hence variability cannot be specified here. Please remove " +
		        ctx->color("var"),
		    fileRange);
	}
	return ir::PtrType::get(isSubtypeVar, subTy, isNonNullable, get_locality(ctx, owner, fileRange), isMulti,
	                        addressSpace.has_value() ? Maybe<ir::AddressSpace>(addressSpace.value().to_ir(ctx)) : None,
	                        ctx->irCtx);
}

AstTypeKind PtrType::type_kind() const { return AstTypeKind::POINTER; }

String PtrType::to_string() const {
	return (isMulti ? (isNonNullable ? "multi![" : "multi:[") : (isNonNullable ? "ptr![" : "ptr:[")) +
	       String(isSubtypeVar ? "var " : "") + type->to_string() +
	       (owner.kind != LocalityKind::NONE ? (", " + owner.to_string()) : "") +
	       (addressSpace.has_value() ? (", " + addressSpace.value().to_string()) : "") + "]";
}

} // namespace qat::ast
