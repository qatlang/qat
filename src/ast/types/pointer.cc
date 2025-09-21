#include "./pointer.hpp"
#include "../../IR/types/pointer.hpp"
#include "../../IR/types/unsigned.hpp"
#include "../../show.hpp"
#include "../expression.hpp"

#include <llvm/Analysis/ConstantFolding.h>
#include <llvm/IR/DerivedTypes.h>

namespace qat::ast {

ir::AddressSpace AddressSpace::to_ir(EmitCtx* ctx) const {
	if (name.value.empty()) {
		if (value->has_type_inferrance()) {
			value->as_type_inferrable()->set_inference_type(ir::UnsignedType::create(32u, ctx->irCtx));
		}
		auto val = value->emit(ctx);
		if (not val->get_ir_type()->is_same(ir::UnsignedType::create(32u, ctx->irCtx))) {
			ctx->Error(
			    "Expected an expression of type " + ctx->color(ir::UnsignedType::create(32u, ctx->irCtx)->to_string()) +
			        ", but got an expression of type " + ctx->color(val->get_ir_type()->to_string()) + " instead",
			    value->fileRange);
		}
		return ir::AddressSpace::from_value(
		    (u32)(*llvm::cast<llvm::ConstantInt>(
		               llvm::ConstantFoldConstant(val->get_llvm_constant(), ctx->irCtx->dataLayout))
		               ->getValue()
		               .getRawData()));
	} else {
		if (name.value == "program" || name.value == "global" || name.value == "local") {
			return ir::AddressSpace::from_name(name.value);
		} else {
			ctx->Error("The address-space " + ctx->color(name.value) + " is unrecognisable", name.range);
			std::unreachable();
		}
	}
}

String AddressSpace::to_string() const { return value ? ("of(" + value->to_string() + ")") : ("of:" + name.value); }

Json AddressSpace::to_json() const {
	return Json()
	    ._("name", name)
	    ._("hasValue", value != nullptr)
	    ._("value", value ? value->to_json() : JsonValue())
	    ._("range", fileRange ? fileRange->to_json_value() : JsonValue());
}

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
	return ir::PtrType::get(isSubtypeVar, subTy, isNonNullable, get_ptr_owner(ctx, owner, fileRange), isMulti,
	                        addressSpace.has_value() ? Maybe<ir::AddressSpace>(addressSpace.value().to_ir(ctx)) : None,
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
	    ._("hasAddressSpace", addressSpace.has_value())
	    ._("addressSpace", addressSpace.has_value() ? addressSpace.value().to_json() : JsonValue())
	    ._("fileRange", fileRange);
}

String PtrType::to_string() const {
	return (isMulti ? (isNonNullable ? "multi![" : "multi:[") : (isNonNullable ? "ptr![" : "ptr:[")) +
	       String(isSubtypeVar ? "var " : "") + type->to_string() +
	       (owner.kind != OwnerKind::NONE ? (", " + owner.to_string()) : "") +
	       (addressSpace.has_value() ? (", " + addressSpace.value().to_string()) : "") + "]";
}

} // namespace qat::ast
