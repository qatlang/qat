#include "./typed.hpp"
#include "../../utils/qat_region.hpp"
#include "../value.hpp"
#include "./type_kind.hpp"

namespace qat::ir {

TypedType::TypedType(ir::Type* _subTy) : subTy(_subTy) {
	while (subTy->is_typed()) {
		subTy = subTy->as_typed()->get_subtype();
	}
	llvmType    = subTy->get_llvm_type();
	linkingName = "type(" + subTy->get_name_for_linking() + ")";
}

TypedType* TypedType::get(Type* _subTy) {
	for (auto* typ : allTypes) {
		if ((typ->type_kind() == TypeKind::typed) && ((TypedType*)typ)->get_subtype()->get_id() == _subTy->get_id()) {
			return (TypedType*)typ;
		}
	}
	return std::construct_at(OwnNormal(TypedType), _subTy);
}

ir::Type* TypedType::get_subtype() const { return subTy->is_typed() ? subTy->as_typed()->get_subtype() : subTy; }

TypeKind TypedType::type_kind() const { return TypeKind::typed; }

String TypedType::to_string() const { return subTy->to_string(); }

Maybe<bool> TypedType::equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const {
	if (first->get_ir_type()->is_typed() && second->get_ir_type()->is_typed()) {
		return first->get_ir_type()->as_typed()->get_subtype()->is_same(
		    second->get_ir_type()->as_typed()->get_subtype());
	}
	return None;
}

Maybe<String> TypedType::to_prerun_generic_string(ir::PrerunValue* val) const {
	// FIXME - The following is stupid, if there is confirmation that the constant's type is already checked
	return val->get_ir_type()->as_typed()->get_subtype()->to_string();
}

} // namespace qat::ir
