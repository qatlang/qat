#include "./pointer.hpp"
#include "../function.hpp"
#include "./region.hpp"

#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>
#include <llvm/Target/TargetMachine.h>

namespace qat::ir {

PtrOwner PtrOwner::of_heap() { return PtrOwner{.owner = nullptr, .ownerTy = OwnerKind::HEAP}; }

PtrOwner PtrOwner::of_static() { return PtrOwner{.owner = nullptr, .ownerTy = OwnerKind::STATIC}; }

PtrOwner PtrOwner::of_none() { return PtrOwner{.owner = nullptr, .ownerTy = OwnerKind::NONE}; }

PtrOwner PtrOwner::of_own(Function* fun) { return PtrOwner{.owner = (void*)fun, .ownerTy = OwnerKind::OWN}; }

PtrOwner PtrOwner::of_self(Type* typ) { return PtrOwner{.owner = (void*)typ, .ownerTy = OwnerKind::SELF}; }

PtrOwner PtrOwner::of_region_type(Region* region) {
	return PtrOwner{.owner = region, .ownerTy = OwnerKind::REGION_TYPE};
}

PtrOwner PtrOwner::of_any_region() { return PtrOwner{.owner = nullptr, .ownerTy = OwnerKind::ANY_REGION}; }

bool PtrOwner::is_same(const PtrOwner& other) const {
	if (ownerTy == other.ownerTy) {
		switch (ownerTy) {
			case OwnerKind::NONE:
			case OwnerKind::STATIC:
			case OwnerKind::HEAP:
			case OwnerKind::ANY_REGION:
				return true;
			case OwnerKind::REGION_TYPE:
				return owner_as_region()->is_same(other.owner_as_region());
			case OwnerKind::OWN:
				return owner_as_parent_function()->get_id() == other.owner_as_parent_function()->get_id();
			case OwnerKind::SELF:
				return owner_as_parent_type()->get_id() == other.owner_as_parent_type()->get_id();
		}
	} else {
		return false;
	}
}

String PtrOwner::to_string() const {
	switch (ownerTy) {
		case OwnerKind::ANY_REGION:
			return "region";
		case OwnerKind::REGION_TYPE:
			return "region(" + owner_as_region()->to_string() + ")";
		case OwnerKind::HEAP:
			return "heap";
		case OwnerKind::NONE:
			return "";
		case OwnerKind::SELF:
			return "''(" + owner_as_parent_type()->to_string() + ")";
		case OwnerKind::OWN:
			return "own(" + owner_as_parent_function()->get_full_name() + ")";
		case OwnerKind::STATIC:
			return "static";
	}
}

u32 AddressSpace::get_number(ir::Ctx* irCtx) const {
	if (name.empty()) {
		return value;
	}
	if (name == "global") {
		return irCtx->dataLayout.getDefaultGlobalsAddressSpace();
	} else if (name == "program") {
		return irCtx->dataLayout.getProgramAddressSpace();
	} else if (name == "local") {
		return irCtx->dataLayout.getAllocaAddrSpace();
	}
	return 0;
}

PtrType::PtrType(bool _isSubtypeVariable, Type* _type, bool _nonNullable, PtrOwner _owner, bool _hasMulti,
                 Maybe<AddressSpace> _addressSpace, ir::Ctx* irCtx)
    : subType(_type), isSubtypeVar(_isSubtypeVariable), owner(_owner), hasMulti(_hasMulti), nonNullable(_nonNullable),
      addressSpace(std::move(_addressSpace)) {
	if (_hasMulti) {
		linkingName = (nonNullable ? "qat'multi![" : "qat'multi:[") + String(isSubtypeVar ? "var " : "") +
		              subType->get_name_for_linking() + (owner.is_none() ? "" : ",") + owner.to_string() +
		              (addressSpace.has_value() ? ("," + addressSpace.value().to_string()) : "") + "]";
		if (llvm::StructType::getTypeByName(irCtx->llctx, linkingName)) {
			llvmType = llvm::StructType::getTypeByName(irCtx->llctx, linkingName);
		} else {
			llvmType = llvm::StructType::create(
			    {llvm::PointerType::get(llvm::Type::getInt8Ty(irCtx->llctx),
			                            addressSpace.has_value() ? addressSpace.value().get_number(irCtx)
			                                                     : irCtx->dataLayout.getProgramAddressSpace()),
			     llvm::Type::getIntNTy(irCtx->llctx,
			                           irCtx->clangTargetInfo->getTypeWidth(irCtx->clangTargetInfo->getSizeType()))},
			    linkingName);
		}
	} else {
		linkingName = (nonNullable ? "qat'ptr![" : "qat'ptr:[") + String(isSubtypeVar ? "var " : "") +
		              subType->get_name_for_linking() + (owner.is_none() ? "" : ",") + owner.to_string() + "]";
		llvmType = llvm::PointerType::get(llvm::Type::getInt8Ty(irCtx->llctx),
		                                  addressSpace.has_value() ? addressSpace.value().get_number(irCtx)
		                                                           : irCtx->dataLayout.getProgramAddressSpace());
	}
}

PtrType* PtrType::get(bool isSubtypeVariable, Type* type, bool nonNullable, PtrOwner owner, bool hasMulti,
                      Maybe<AddressSpace> addressSpace, ir::Ctx* irCtx) {
	for (auto* typ : allTypes) {
		if (typ->is_ptr()) {
			if (typ->as_ptr()->get_subtype()->is_same(type) &&
			    (typ->as_ptr()->is_subtype_variable() == isSubtypeVariable) &&
			    typ->as_ptr()->get_owner().is_same(owner) && (typ->as_ptr()->is_multi() == hasMulti) &&
			    (typ->as_ptr()->nonNullable == nonNullable) &&
			    ir::AddressSpace::compare(typ->as_ptr()->get_address_space(), addressSpace)) {
				return typ->as_ptr();
			}
		}
	}
	return std::construct_at(OwnNormal(PtrType), isSubtypeVariable, type, nonNullable, owner, hasMulti,
	                         std::move(addressSpace), irCtx);
}

bool PtrType::is_subtype_variable() const { return isSubtypeVar; }

bool PtrType::is_type_sized() const { return true; }

bool PtrType::has_prerun_default_value() const { return not nonNullable; }

PrerunValue* PtrType::get_prerun_default_value(ir::Ctx* irCtx) {
	if (has_prerun_default_value()) {
		if (is_multi()) {
			return ir::PrerunValue::get(llvm::ConstantAggregateZero::get(llvmType), this);
		} else {
			return ir::PrerunValue::get(llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(get_llvm_type())),
			                            this);
		}
	} else {
		irCtx->Error("Type " + irCtx->color(to_string()) + " do not have a default value", None);
		return nullptr;
	}
}

bool PtrType::has_simple_copy() const { return true; }

bool PtrType::has_simple_move() const { return not nonNullable; }

bool PtrType::is_multi() const { return hasMulti; }

bool PtrType::is_nullable() const { return not nonNullable; }

bool PtrType::is_non_nullable() const { return nonNullable; }

Type* PtrType::get_subtype() const { return subType; }

Maybe<AddressSpace> const& PtrType::get_address_space() const { return addressSpace; }

PtrOwner PtrType::get_owner() const { return owner; }

TypeKind PtrType::type_kind() const { return TypeKind::POINTER; }

String PtrType::to_string() const {
	return String(is_multi() ? (nonNullable ? "multi![" : "multi:[") : (nonNullable ? "ptr![" : "ptr:[")) +
	       String(is_subtype_variable() ? "var " : "") + subType->to_string() + (owner.is_none() ? "" : " ") +
	       owner.to_string() + "]";
}

} // namespace qat::ir
