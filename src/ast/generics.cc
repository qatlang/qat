#include "./generics.hpp"
#include "expression.hpp"
#include "node.hpp"
#include "types/integer.hpp"
#include "types/unsigned.hpp"

namespace qat::ast {

bool FillGeneric::is_type() const { return kind == FillGenericKind::typed; }

bool FillGeneric::is_prerun() const { return kind == FillGenericKind::prerun; }

Type* FillGeneric::as_type() const { return (Type*)data; }

PrerunExpression* FillGeneric::as_prerun() const { return (PrerunExpression*)data; }

FileRange const& FillGeneric::get_range() const { return as_type()->fileRange; }

ir::GenericToFill* FillGeneric::toFill(EmitCtx* ctx) const {
	SHOW("ToFill called")
	if (is_type()) {
		if (as_type()->type_kind() == AstTypeKind::INTEGER) {
			((IntegerType*)as_type())->isPartOfGeneric = true;
		} else if (as_type()->type_kind() == AstTypeKind::UNSIGNED_INTEGER) {
			((UnsignedType*)as_type())->isPartOfGeneric = true;
		}
		return ir::GenericToFill::GetType(as_type()->emit(ctx), as_type()->fileRange);
	} else {
		return ir::GenericToFill::GetPrerun(as_prerun()->emit(ctx), as_prerun()->fileRange);
	}
}

String FillGeneric::to_string() const {
	if (is_type()) {
		return as_type()->to_string();
	} else {
		return as_prerun()->to_string();
	}
}

Json FillGeneric::to_json() const {
	return Json()
		._("kind", (kind == FillGenericKind::typed) ? "typed" : "prerun")
		._("type", is_type() ? as_type()->to_json() : Json())
		._("constant", is_prerun() ? as_prerun()->to_json() : Json());
}

} // namespace qat::ast