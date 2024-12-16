#include "./mark_owner.hpp"
#include "../emit_ctx.hpp"

namespace qat::ast {

String mark_owner_to_string(MarkOwnType ownType) {
	switch (ownType) {
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

ir::MarkOwner get_mark_owner(EmitCtx* ctx, MarkOwnType ownType, Maybe<ir::Type*> ownerVal, FileRange fileRange) {
	switch (ownType) {
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

} // namespace qat::ast
