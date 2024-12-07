#include "./pointer.hpp"
#include "../../IR/types/pointer.hpp"
#include "../../show.hpp"

#include <llvm/IR/DerivedTypes.h>

namespace qat::ast {

void MarkType::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
								   EmitCtx* ctx) {
	type->update_dependencies(phase, ir::DependType::partial, ent, ctx);
	if (ownerTyTy.has_value()) {
		ownerTyTy.value()->update_dependencies(phase, ir::DependType::partial, ent, ctx);
	}
}

Maybe<usize> MarkType::getTypeSizeInBits(EmitCtx* ctx) const {
	return (usize)(ctx->mod->get_llvm_module()->getDataLayout().getTypeAllocSizeInBits(
		isSlice ? llvm::cast<llvm::Type>(llvm::StructType::create(
					  {llvm::PointerType::get(llvm::Type::getInt8Ty(ctx->irCtx->llctx),
											  ctx->irCtx->dataLayout->getProgramAddressSpace()),
					   llvm::Type::getInt64Ty(ctx->irCtx->llctx)}))
				: llvm::cast<llvm::Type>(llvm::PointerType::get(llvm::Type::getInt8Ty(ctx->irCtx->llctx),
																ctx->irCtx->dataLayout->getProgramAddressSpace()))));
}

String MarkType::pointerOwnerToString() const {
	switch (ownTyp) {
		case MarkOwnType::type:
			return "type";
		case MarkOwnType::typeParent:
			return "typeParent";
		case MarkOwnType::function:
			return "function";
		case MarkOwnType::anonymous:
			return "anonymous";
		case MarkOwnType::heap:
			return "heap";
		case MarkOwnType::region:
			return "region";
		case MarkOwnType::anyRegion:
			return "anyRegion";
	}
}

ir::MarkOwner MarkType::getPointerOwner(EmitCtx* ctx, Maybe<ir::Type*> ownerVal) const {
	switch (ownTyp) {
		case MarkOwnType::type:
			return ir::MarkOwner::of_type(ownerVal.value());
		case MarkOwnType::typeParent: {
			if (ctx->has_member_parent()) {
				return ir::MarkOwner::of_parent_instance(ctx->get_member_parent()->get_parent_type());
			} else {
				ctx->Error("No parent type or skill found", fileRange);
			}
		}
		case MarkOwnType::anonymous:
			return ir::MarkOwner::of_anonymous();
		case MarkOwnType::heap:
			return ir::MarkOwner::of_heap();
		case MarkOwnType::function:
			return ir::MarkOwner::of_parent_function(ctx->get_fn());
		case MarkOwnType::region:
			return ir::MarkOwner::of_region(ownerVal.value()->as_region());
		case MarkOwnType::anyRegion:
			return ir::MarkOwner::of_any_region();
	}
}

ir::Type* MarkType::emit(EmitCtx* ctx) {
	if (ownTyp == MarkOwnType::function) {
		if (!ctx->get_fn()) {
			ctx->Error("This pointer type is not inside a function and hence cannot have function ownership",
					   fileRange);
		}
	} else if (ownTyp == MarkOwnType::typeParent) {
		if (!(ctx->has_member_parent())) {
			ctx->Error("No parent type found in scope and hence the pointer "
					   "cannot be owned by the parent type instance",
					   fileRange);
		}
	}
	Maybe<ir::Type*> ownerVal;
	if (ownTyp == MarkOwnType::type) {
		if (!ownerTyTy) {
			ctx->Error("Expected a type to be provided for pointer ownership", fileRange);
		}
		auto* typVal = ownerTyTy.value()->emit(ctx);
		if (typVal->is_region()) {
			ctx->Error("The type provided is a region and hence pointer ownership has to be " +
						   ctx->color("'region(" + typVal->to_string() = ")") + " or " + ctx->color("'region"),
					   fileRange);
		}
		ownerVal = typVal;
	} else if (ownTyp == MarkOwnType::region) {
		if (ownerTyTy) {
			auto* regTy = ownerTyTy.value()->emit(ctx);
			if (!regTy->is_region()) {
				ctx->Error("The provided type is not a region type and hence pointer "
						   "owner cannot be " +
							   ctx->color("region"),
						   fileRange);
			}
			ownerVal = regTy;
		}
	}
	auto subTy = type->emit(ctx);
	if (isSubtypeVar && subTy->is_function()) {
		ctx->Error(
			"The subtype of this pointer type is a function type, and hence variability cannot be specified here. Please remove " +
				ctx->color("var"),
			fileRange);
	}
	return ir::MarkType::get(isSubtypeVar, subTy, isNonNullable, getPointerOwner(ctx, ownerVal), isSlice, ctx->irCtx);
}

AstTypeKind MarkType::type_kind() const { return AstTypeKind::POINTER; }

Json MarkType::to_json() const {
	return Json()
		._("typeKind", "mark")
		._("isSlice", isSlice)
		._("isSubtypeVariable", isSubtypeVar)
		._("subType", type->to_json())
		._("ownerKind", pointerOwnerToString())
		._("hasOwnerType", ownerTyTy.has_value())
		._("ownerType", ownerTyTy.has_value() ? ownerTyTy.value()->to_json() : Json())
		._("fileRange", fileRange);
}

String MarkType::to_string() const {
	Maybe<String> ownerStr;
	switch (ownTyp) {
		case MarkOwnType::type: {
			ownerStr = String("type(") + ownerTyTy.value()->to_string() + ")";
			break;
		}
		case MarkOwnType::typeParent: {
			ownerStr = "''";
			break;
		}
		case MarkOwnType::heap: {
			ownerStr = "heap";
			break;
		}
		case MarkOwnType::function: {
			ownerStr = "own";
			break;
		}
		case MarkOwnType::region: {
			ownerStr = String("region(") + ownerTyTy.value()->to_string() + ")";
			break;
		}
		case MarkOwnType::anyRegion: {
			ownerStr = "region";
			break;
		}
		default:;
	}
	return (isSlice ? "slice:[" : "mark:[") + String(isSubtypeVar ? "var " : "") + type->to_string() +
		   (ownerStr.has_value() ? (", " + ownerStr.value()) : "") + "]";
}

} // namespace qat::ast
