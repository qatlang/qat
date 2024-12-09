#include "./self_type.hpp"
#include "../../IR/types/reference.hpp"

namespace qat::ast {

ir::Type* SelfType::emit(EmitCtx* ctx) {
	if (ctx->has_member_parent() || ctx->has_opaque_parent()) {
		if (isJustType) {
			return ctx->has_member_parent() ? ctx->get_member_parent()->get_parent_type() : ctx->get_opaque_parent();
		} else {
			if (canBeSelfInstance) {
				return ir::ReferenceType::get(isVarRef,
				                              ctx->has_member_parent() ? ctx->get_member_parent()->get_parent_type()
				                                                       : ctx->get_opaque_parent(),
				                              ctx->irCtx);
			} else {
				ctx->Error("The " + ctx->color("''") +
				               " type can only be used as given type in normal and variation methods",
				           fileRange);
			}
		}
	} else {
		ctx->Error("No parent type found in this scope.", fileRange);
	}
	return nullptr;
}

Json SelfType::to_json() const {
	return Json()._("nodeType", "selfType")._("isJustType", isJustType)._("fileRange", fileRange);
}

String SelfType::to_string() const { return isJustType ? "self" : "''"; }

} // namespace qat::ast