#include "./pointer.hpp"
#include "../../IR/types/pointer.hpp"
#include "../../show.hpp"

#include <llvm/IR/DerivedTypes.h>

namespace qat::ast {

void PtrType::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent,
                                  EmitCtx* ctx) {
	type->update_dependencies(phase, ir::DependType::partial, ent, ctx);
	if (owner.candidate) {
		owner.candidate->update_dependencies(phase, ir::DependType::partial, ent, ctx);
	}
}

Maybe<usize> PtrType::get_type_bitsize(EmitCtx* ctx) const {
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
	return ir::PtrType::get(isSubtypeVar, subTy, isNonNullable, get_ptr_owner(ctx, owner, fileRange), isMulti,
	                        ctx->irCtx);
}

AstTypeKind PtrType::type_kind() const { return AstTypeKind::POINTER; }

Json PtrType::to_json() const {
	return Json()
	    ._("typeKind", "pointer")
	    ._("isMulti", isMulti)
	    ._("isNonNullable", isNonNullable)
	    ._("isSubtypeVariable", isSubtypeVar)
	    ._("subType", type->to_json())
	    ._("owner", owner.to_json())
	    ._("fileRange", fileRange);
}

String PtrType::to_string() const {
	return (isMulti ? (isNonNullable ? "multi![" : "multi:[") : (isNonNullable ? "ptr![" : "ptr:[")) +
	       String(isSubtypeVar ? "var " : "") + type->to_string() +
	       (owner.kind != PtrOwnType::anonymous ? (", " + owner.to_string()) : "") + "]";
}

} // namespace qat::ast
