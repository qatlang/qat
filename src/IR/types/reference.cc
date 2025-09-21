#include "./reference.hpp"
#include "../context.hpp"

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>

namespace qat::ir {

Vec<RefType*> RefType::allRefTypes = {};

RefType::RefType(bool _isSubtypeVariable, Type* _type, ir::Ctx* irCtx)
    : subType(_type), isSubVariable(_isSubtypeVariable) {
	if (subType->is_type_sized()) {
		llvmType = llvm::PointerType::get(subType->get_llvm_type(), irCtx->dataLayout.getProgramAddressSpace());
	} else {
		llvmType =
		    llvm::PointerType::get(llvm::Type::getInt8Ty(irCtx->llctx), irCtx->dataLayout.getProgramAddressSpace());
	}
	linkingName = "qat'ref:[" + String(isSubVariable ? "var " : "") + subType->get_name_for_linking() + "]";
	allRefTypes.push_back(this);
}

RefType* RefType::get(bool isSubtypeVariable, Type* subType, ir::Ctx* irCtx) {
	for (auto* typ : allRefTypes) {
		if (typ->get_subtype()->is_same(subType) && (typ->has_variability() == isSubtypeVariable)) {
			return typ;
		}
	}
	return std::construct_at(OwnNormal(RefType), isSubtypeVariable, subType, irCtx);
}

Type* RefType::get_subtype() const { return subType; }

bool RefType::has_variability() const { return isSubVariable; }

bool RefType::is_type_sized() const { return true; }

TypeKind RefType::type_kind() const { return TypeKind::REFERENCE; }

String RefType::to_string() const { return "ref:[" + String(isSubVariable ? "var " : "") + subType->to_string() + "]"; }

} // namespace qat::ir
