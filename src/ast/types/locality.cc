#include "./locality.hpp"
#include "../../IR/method.hpp"
#include "../emit_ctx.hpp"
#include "./qat_type.hpp"

namespace qat::ast {

String Locality::to_string() const {
	switch (kind) {
		case LocalityKind::STATIC:
			return "static";
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
		case LocalityKind::OWN:
			return "own";
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
		case LocalityKind::NONE:
			return ir::Locality::none();
		case LocalityKind::HEAP:
			return ir::Locality::in_heap();
		case LocalityKind::STATIC:
			return ir::Locality::in_static();
		case LocalityKind::OWN:
			return ir::Locality::in_own();
		case LocalityKind::REGION_TYPE:
			return ir::Locality::in_region_type(originVal->as_region());
		case LocalityKind::PRERUN:
			return ir::Locality::in_prerun();
		case LocalityKind::ANY_REGION:
			return ir::Locality::in_any_region();
	}
}

} // namespace qat::ast
