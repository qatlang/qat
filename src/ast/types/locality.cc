#include "./locality.hpp"
#include "../../IR/method.hpp"
#include "../emit_ctx.hpp"
#include "./qat_type.hpp"

namespace qat::ast {

String Locality::to_string() const {
	switch (kind) {
		case LocalityKind::STATIC:
			return "static";
		case LocalityKind::SELF_INSTANCE:
			return "''";
		case LocalityKind::OWN:
			return "own";
		case LocalityKind::NONE:
			return "";
		case LocalityKind::HEAP:
			return "heap";
		case LocalityKind::REGION_TYPE:
			return "region(" + candidate->to_string() + ")";
		case LocalityKind::ANY_REGION:
			return "region";
		case LocalityKind::PRERUN:
			return "pre";
	}
}

String locality_to_string(LocalityKind locality) {
	switch (locality) {
		case LocalityKind::STATIC:
			return "static";
		case LocalityKind::SELF_INSTANCE:
			return "typeParent";
		case LocalityKind::OWN:
			return "function";
		case LocalityKind::NONE:
			return "anonymous";
		case LocalityKind::HEAP:
			return "heap";
		case LocalityKind::REGION_TYPE:
			return "region";
		case LocalityKind::ANY_REGION:
			return "anyRegion";
		case LocalityKind::PRERUN:
			return "prerun";
	}
}

ir::Locality get_locality(EmitCtx* ctx, Locality locality, FileRangePtr fileRange) {
	if (locality.kind == LocalityKind::OWN) {
		if (not ctx->get_fn()) {
			ctx->Error("This pointer type is not inside a function and hence cannot have the " + ctx->color("own") +
			               " locality.",
			           fileRange);
		}
	} else if (locality.kind == LocalityKind::SELF_INSTANCE) {
		if (not ctx->has_member_parent()) {
			ctx->Error("No parent type found in scope and hence the pointer "
			           "cannot be owned by the parent type instance",
			           fileRange);
		}
	}
	ir::Type* originVal = nullptr;
	if (locality.kind == LocalityKind::REGION_TYPE) {
		if (locality.candidate) {
			auto* regTy = locality.candidate->emit(ctx);
			if (not regTy->is_region()) {
				ctx->Error("The provided type is not a region type and hence the origin of the pointer "
				           "locality cannot be " +
				               ctx->color("region"),
				           fileRange);
			}
			originVal = regTy;
		}
	}
	switch (locality.kind) {
		case LocalityKind::SELF_INSTANCE: {
			if (ctx->has_member_parent()) {
				return ir::Locality::in_self(ctx->get_member_parent()->get_parent_type());
			} else {
				ctx->Error("No parent type or skill found", fileRange);
			}
		}
		case LocalityKind::NONE:
			return ir::Locality::none();
		case LocalityKind::HEAP:
			return ir::Locality::in_heap();
		case LocalityKind::STATIC:
			return ir::Locality::in_static();
		case LocalityKind::OWN:
			return ir::Locality::in_own(ctx->get_fn());
		case LocalityKind::REGION_TYPE:
			return ir::Locality::in_region_type(originVal->as_region());
		case LocalityKind::PRERUN:
			return ir::Locality::in_prerun();
		case LocalityKind::ANY_REGION:
			return ir::Locality::in_any_region();
	}
}

} // namespace qat::ast
