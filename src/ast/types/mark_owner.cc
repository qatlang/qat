#include "./mark_owner.hpp"
#include "../emit_ctx.hpp"

namespace qat::ast {

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

ir::PtrOwner get_ptr_owner(EmitCtx* ctx, PtrOwnType ownType, Maybe<ir::Type*> ownerVal, FileRange fileRange) {
	switch (ownType) {
		case PtrOwnType::type:
			return ir::PtrOwner::of_type(ownerVal.value());
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
			return ir::PtrOwner::of_region(ownerVal.value()->as_region());
		case PtrOwnType::anyRegion:
			return ir::PtrOwner::of_any_region();
	}
}

} // namespace qat::ast
