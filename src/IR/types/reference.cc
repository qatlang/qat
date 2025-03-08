#include "./reference.hpp"
#include "../context.hpp"

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>

namespace qat::ir {

RefType::RefType(bool _isSubtypeVariable, Type* _type, ir::Ctx* irCtx)
    : subType(_type), isSubVariable(_isSubtypeVariable) {
	if (subType->is_type_sized()) {
		llvmType = llvm::PointerType::get(subType->get_llvm_type(), irCtx->dataLayout.getProgramAddressSpace());
	} else {
		llvmType =
		    llvm::PointerType::get(llvm::Type::getInt8Ty(irCtx->llctx), irCtx->dataLayout.getProgramAddressSpace());
	}
	linkingName = "qat'@" + String(isSubVariable ? "var[" : "[") + subType->get_name_for_linking() + "]";
}

RefType* RefType::get(bool _isSubtypeVariable, Type* _subtype, ir::Ctx* irCtx) {
	for (auto* typ : allTypes) {
		if (typ->is_ref()) {
			if (typ->as_ref()->get_subtype()->is_same(_subtype) &&
			    (typ->as_ref()->has_variability() == _isSubtypeVariable)) {
				return typ->as_ref();
			}
		}
	}
	return std::construct_at(OwnNormal(RefType), _isSubtypeVariable, _subtype, irCtx);
}

Type* RefType::get_subtype() const { return subType; }

bool RefType::has_variability() const { return isSubVariable; }

bool RefType::is_type_sized() const { return true; }

TypeKind RefType::type_kind() const { return TypeKind::REFERENCE; }

String RefType::to_string() const { return "@" + String(isSubVariable ? "var[" : "[") + subType->to_string() + "]"; }

} // namespace qat::ir
