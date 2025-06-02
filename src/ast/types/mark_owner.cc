#include "./mark_owner.hpp"
#include "../emit_ctx.hpp"
#include "./qat_type.hpp"

namespace qat::ast {

String PtrOwner::to_string() const {
	switch (kind) {
		case PtrOwnType::type:
			return "type(" + candidate->to_string() + ")";
		case PtrOwnType::typeParent:
			return "''";
		case PtrOwnType::function:
			return "own";
		case PtrOwnType::anonymous:
			return "";
		case PtrOwnType::heap:
			return "heap";
		case PtrOwnType::region:
			return "region(" + candidate->to_string() + ")";
		case PtrOwnType::anyRegion:
			return "region";
	}
}

Json PtrOwner::to_json() const {
	return Json()
	    ._("kind", ptr_owner_to_string(kind))
	    ._("hasAssociatedType", candidate != nullptr)
	    ._("associatedType", candidate ? candidate->to_json() : JsonValue());
}

String ptr_owner_to_string(PtrOwnType ownType) {
	switch (ownType) {
		case PtrOwnType::type:
			return "type";
		case PtrOwnType::typeParent:
			return "typeParent";
		case PtrOwnType::function:
			return "function";
		case PtrOwnType::anonymous:
			return "anonymous";
		case PtrOwnType::heap:
			return "heap";
		case PtrOwnType::region:
			return "region";
		case PtrOwnType::anyRegion:
			return "anyRegion";
	}
}

ir::PtrOwner get_ptr_owner(EmitCtx* ctx, PtrOwner owner, FileRange fileRange) {

	if (owner.kind == PtrOwnType::function) {
		if (not ctx->get_fn()) {
			ctx->Error("This pointer type is not inside a function and hence cannot have function ownership",
			           fileRange);
		}
	} else if (owner.kind == PtrOwnType::typeParent) {
		if (not ctx->has_member_parent()) {
			ctx->Error("No parent type found in scope and hence the pointer "
			           "cannot be owned by the parent type instance",
			           fileRange);
		}
	}
	ir::Type* ownerVal = nullptr;
	if (owner.kind == PtrOwnType::type) {
		if (not owner.candidate) {
			ctx->Error("Expected a type to be provided for pointer ownership", fileRange);
		}
		auto* typVal = owner.candidate->emit(ctx);
		if (typVal->is_region()) {
			ctx->Error("The type provided is a region and hence pointer ownership has to be " +
			               ctx->color("'region(" + typVal->to_string() = ")") + " or " + ctx->color("'region"),
			           fileRange);
		}
		ownerVal = typVal;
	} else if (owner.kind == PtrOwnType::region) {
		if (owner.candidate) {
			auto* regTy = owner.candidate->emit(ctx);
			if (not regTy->is_region()) {
				ctx->Error("The provided type is not a region type and hence pointer "
				           "owner cannot be " +
				               ctx->color("region"),
				           fileRange);
			}
			ownerVal = regTy;
		}
	}
	switch (owner.kind) {
		case PtrOwnType::type:
			return ir::PtrOwner::of_type(ownerVal);
		case PtrOwnType::typeParent: {
			if (ctx->has_member_parent()) {
				return ir::PtrOwner::of_parent_instance(ctx->get_member_parent()->get_parent_type());
			} else {
				ctx->Error("No parent type or skill found", fileRange);
			}
		}
		case PtrOwnType::anonymous:
			return ir::PtrOwner::of_anonymous();
		case PtrOwnType::heap:
			return ir::PtrOwner::of_heap();
		case PtrOwnType::function:
			return ir::PtrOwner::of_parent_function(ctx->get_fn());
		case PtrOwnType::region:
			return ir::PtrOwner::of_region(ownerVal->as_region());
		case PtrOwnType::anyRegion:
			return ir::PtrOwner::of_any_region();
	}
}

} // namespace qat::ast
