#include "./reference.hpp"
#include "../context.hpp"

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>

namespace qat::ir {

Vec<RefType*> RefType::allRefTypes = {};

RefType::RefType(bool _isSubtypeVariable, Type* _type, Maybe<AddressSpace> _addressSpace, ir::Ctx* irCtx)
    : subType(_type), isSubVariable(_isSubtypeVariable), addressSpace(std::move(_addressSpace)) {
	llvmType =
	    llvm::PointerType::get(irCtx->llctx, addressSpace.has_value() ? addressSpace.value().get_number(irCtx)
	                                                                  : irCtx->dataLayout.getProgramAddressSpace());
	linkingName = "qat'ref:[" + String(isSubVariable ? "var " : "") + subType->get_name_for_linking() +
	              (addressSpace.has_value() ? ("," + addressSpace.value().to_string()) : "") + "]";
	allRefTypes.push_back(this);
}

RefType* RefType::get(bool isSubtypeVariable, Type* subType, Maybe<AddressSpace> addressSpace, ir::Ctx* irCtx) {
	for (auto* typ : allRefTypes) {
		if (typ->get_subtype()->is_same(subType) && (typ->has_variability() == isSubtypeVariable) &&
		    ir::AddressSpace::compare(typ->get_address_space(), addressSpace)) {
			return typ;
		}
	}
	return std::construct_at(OwnNormal(RefType), isSubtypeVariable, subType, std::move(addressSpace), irCtx);
}

Type* RefType::get_subtype() const { return subType; }

bool RefType::has_variability() const { return isSubVariable; }

bool RefType::is_type_sized() const { return true; }

TypeKind RefType::type_kind() const { return TypeKind::REFERENCE; }

String RefType::to_string() const {
	return "ref:[" + String(isSubVariable ? "var " : "") + subType->to_string() +
	       (addressSpace.has_value() ? (", " + addressSpace.value().to_string()) : "") + "]";
}

} // namespace qat::ir
