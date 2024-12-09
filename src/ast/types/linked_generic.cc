#include "./linked_generic.hpp"
#include "../../utils/utils.hpp"
#include "generic_abstract.hpp"
#include "prerun_generic.hpp"
#include "type_kind.hpp"
#include "typed_generic.hpp"

namespace qat::ast {

ir::Type* LinkedGeneric::emit(EmitCtx* ctx) {
	if (genAbs->is_typed() || (genAbs->as_typed() && genAbs->as_prerun()->getType()->is_typed())) {
		if (genAbs->isSet()) {
			if (genAbs->is_typed()) {
				return genAbs->as_typed()->get_type();
			} else if (genAbs->as_typed()) {
				return genAbs->as_prerun()->getPrerun()->get_ir_type()->as_typed()->get_subtype();
			} else {
				ctx->Error("Invalid generic kind", fileRange);
			}
		} else {
			if (genAbs->is_typed()) {
				ctx->Error("No type set for the " + utils::number_to_position(genAbs->getIndex()) +
				               " Generic Parameter " + ctx->color(genAbs->get_name().value),
				           fileRange);
			} else if (genAbs->as_typed()) {
				ctx->Error("No prerun expression set for the " + utils::number_to_position(genAbs->getIndex()) +
				               " Generic Parameter " + ctx->color(genAbs->get_name().value),
				           fileRange);
			} else {
				ctx->Error("Invalid generic kind", fileRange);
			}
		}
	} else if (genAbs->as_typed()) {
		ctx->Error(utils::number_to_position(genAbs->getIndex()) + " Generic Parameter " +
		               ctx->color(genAbs->get_name().value) + " is a normal prerun expression with type " +
		               genAbs->as_prerun()->getType()->to_string() + " and hence cannot be used as a type",
		           fileRange);
	} else {
		ctx->Error("Invalid generic kind", fileRange);
	}
}

AstTypeKind LinkedGeneric::type_kind() const { return AstTypeKind::LINKED_GENERIC; }

String LinkedGeneric::to_string() const { return genAbs->get_name().value; }

Json LinkedGeneric::to_json() const {
	return Json()._("genericParameter", genAbs->get_name())._("fileRange", fileRange);
}

} // namespace qat::ast