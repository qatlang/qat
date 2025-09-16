#include "./pointer_owner.hpp"
#include "../../IR/method.hpp"
#include "../emit_ctx.hpp"
#include "./qat_type.hpp"

namespace qat::ast {

String PtrOwner::to_string() const {
	switch (kind) {
		case OwnerKind::STATIC:
			return "static";
		case OwnerKind::SELF_INSTANCE:
			return "''";
		case OwnerKind::OWN:
			return "own";
		case OwnerKind::NONE:
			return "";
		case OwnerKind::HEAP:
			return "heap";
		case OwnerKind::REGION_TYPE:
			return "region(" + candidate->to_string() + ")";
		case OwnerKind::ANY_REGION:
			return "region";
	}
}

Json PtrOwner::to_json() const {
	return Json()
	    ._("kind", ptr_owner_to_string(kind))
	    ._("hasAssociatedType", candidate != nullptr)
	    ._("associatedType", candidate ? candidate->to_json() : JsonValue());
}

String ptr_owner_to_string(OwnerKind ownType) {
	switch (ownType) {
		case OwnerKind::STATIC:
			return "static";
		case OwnerKind::SELF_INSTANCE:
			return "typeParent";
		case OwnerKind::OWN:
			return "function";
		case OwnerKind::NONE:
			return "anonymous";
		case OwnerKind::HEAP:
			return "heap";
		case OwnerKind::REGION_TYPE:
			return "region";
		case OwnerKind::ANY_REGION:
			return "anyRegion";
	}
}

ir::PtrOwner get_ptr_owner(EmitCtx* ctx, PtrOwner owner, FileRangePtr fileRange) {
	if (owner.kind == OwnerKind::OWN) {
		if (not ctx->get_fn()) {
			ctx->Error("This pointer type is not inside a function and hence cannot have function ownership",
			           fileRange);
		}
	} else if (owner.kind == OwnerKind::SELF_INSTANCE) {
		if (not ctx->has_member_parent()) {
			ctx->Error("No parent type found in scope and hence the pointer "
			           "cannot be owned by the parent type instance",
			           fileRange);
		}
	}
	ir::Type* ownerVal = nullptr;
	if (owner.kind == OwnerKind::REGION_TYPE) {
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
		case OwnerKind::SELF_INSTANCE: {
			if (ctx->has_member_parent()) {
				return ir::PtrOwner::of_self(ctx->get_member_parent()->get_parent_type());
			} else {
				ctx->Error("No parent type or skill found", fileRange);
			}
		}
		case OwnerKind::NONE:
			return ir::PtrOwner::of_none();
		case OwnerKind::HEAP:
			return ir::PtrOwner::of_heap();
		case OwnerKind::STATIC:
			return ir::PtrOwner::of_static();
		case OwnerKind::OWN:
			return ir::PtrOwner::of_own(ctx->get_fn());
		case OwnerKind::REGION_TYPE:
			return ir::PtrOwner::of_region_type(ownerVal->as_region());
		case OwnerKind::ANY_REGION:
			return ir::PtrOwner::of_any_region();
	}
}

} // namespace qat::ast
