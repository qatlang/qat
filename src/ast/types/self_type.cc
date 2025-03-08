#include "./self_type.hpp"
#include "../../IR/types/reference.hpp"

namespace qat::ast {

ir::Type* SelfType::emit(EmitCtx* ctx) {
	if (ctx->has_member_parent() || ctx->has_opaque_parent()) {
		if (isJustType) {
			return ctx->has_member_parent() ? ctx->get_member_parent()->get_parent_type() : ctx->get_opaque_parent();
		}
		if (not canBeSelfInstance) {
			ctx->Error("The " + ctx->color("''") +
			               " type can only be used as the given type in normal and variation methods",
			           fileRange);
		}
		return ir::RefType::get(
		    isVarRef, ctx->has_member_parent() ? ctx->get_member_parent()->get_parent_type() : ctx->get_opaque_parent(),
		    ctx->irCtx);
	} else if (ctx->has_skill()) {
		ctx->Error(
		    "The type " + ctx->color(to_string()) + " is not allowed in skills, as the parent type" +
		        (isJustType ? "" : " of the instance") +
		        " that it represents cannot be known while defining the skill, and can only be known in the implementation"
		        " of the skill for that specific type. Skills are predictable interfaces to common behaviour, so this is not allowed.",
		    fileRange);
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
