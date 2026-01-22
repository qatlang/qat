#include "./pointer.hpp"
#include "../function.hpp"
#include "./region.hpp"

#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>
#include <llvm/Target/TargetMachine.h>

namespace qat::ir {

Vec<PtrType*> PtrType::allPtrTypes = {};

Locality Locality::in_heap() { return Locality{.origin = nullptr, .locality = LocalityKind::HEAP}; }

Locality Locality::in_static() { return Locality{.origin = nullptr, .locality = LocalityKind::STATIC}; }

Locality Locality::none() { return Locality{.origin = nullptr, .locality = LocalityKind::NONE}; }

Locality Locality::in_own(Function* fun) { return Locality{.origin = (void*)fun, .locality = LocalityKind::OWN}; }

Locality Locality::in_self(Type* typ) { return Locality{.origin = (void*)typ, .locality = LocalityKind::SELF}; }

Locality Locality::in_region_type(Region* region) {
	return Locality{.origin = region, .locality = LocalityKind::REGION_TYPE};
}

Locality Locality::in_any_region() { return Locality{.origin = nullptr, .locality = LocalityKind::ANY_REGION}; }

Locality Locality::in_prerun() { return Locality{.origin = nullptr, .locality = LocalityKind::PRERUN}; }

bool Locality::is_same(const Locality& other) const {
	if (locality == other.locality) {
		switch (locality) {
			case LocalityKind::NONE:
			case LocalityKind::STATIC:
			case LocalityKind::HEAP:
			case LocalityKind::ANY_REGION:
			case LocalityKind::PRERUN:
			case LocalityKind::OWN:
			case LocalityKind::USE:
			case LocalityKind::ATOMIC:
				return true;
			case LocalityKind::REGION_TYPE:
				return origin_as_region()->is_same(other.origin_as_region());
			case LocalityKind::SELF:
				return origin_as_parent_type()->get_id() == other.origin_as_parent_type()->get_id();
		}
	} else {
		return false;
	}
}

String Locality::to_string() const {
	switch (locality) {
		case LocalityKind::ANY_REGION:
			return "region";
		case LocalityKind::REGION_TYPE:
			return "region(" + origin_as_region()->to_string() + ")";
		case LocalityKind::HEAP:
			return "heap";
		case LocalityKind::NONE:
			return "";
		case LocalityKind::SELF:
			return "''(" + origin_as_parent_type()->to_string() + ")";
		case LocalityKind::OWN:
			return "own(" + owner_as_parent_function()->get_full_name() + ")";
		case LocalityKind::STATIC:
			return "static";
		case LocalityKind::PRERUN:
			return "pre";
		case LocalityKind::USE:
			return "use";
		case LocalityKind::ATOMIC:
			return "atomic";
	}
}

PtrType::PtrType(bool _isSubtypeVariable, Type* _type, bool _nonNullable, Locality _locality, bool _hasMulti,
                 Maybe<AddressSpace> _addressSpace, ir::Ctx* irCtx)
    : subType(_type), isSubtypeVar(_isSubtypeVariable), locality(_locality), hasMulti(_hasMulti),
      nonNullable(_nonNullable), addressSpace(std::move(_addressSpace)) {
	if (_hasMulti) {
		linkingName = (nonNullable ? "qat'multi![" : "qat'multi:[") + String(isSubtypeVar ? "var " : "") +
		              subType->get_name_for_linking() + (locality.is_none() ? "" : ",") + locality.to_string() +
		              (addressSpace.has_value() ? ("," + addressSpace.value().to_string()) : "") + "]";
		if (llvm::StructType::getTypeByName(irCtx->llctx, linkingName)) {
			llvmType = llvm::StructType::getTypeByName(irCtx->llctx, linkingName);
		} else {
			llvmType = llvm::StructType::create(
			    {llvm::PointerType::get(irCtx->llctx, addressSpace.has_value()
			                                              ? addressSpace.value().get_number(irCtx)
			                                              : irCtx->dataLayout.getProgramAddressSpace()),
			     llvm::Type::getIntNTy(irCtx->llctx,
			                           irCtx->clangTargetInfo->getTypeWidth(irCtx->clangTargetInfo->getSizeType()))},
			    linkingName);
		}
	} else {
		linkingName = (nonNullable ? "qat'ptr![" : "qat'ptr:[") + String(isSubtypeVar ? "var " : "") +
		              subType->get_name_for_linking() + (locality.is_none() ? "" : ",") + locality.to_string() + "]";
		llvmType =
		    llvm::PointerType::get(irCtx->llctx, addressSpace.has_value() ? addressSpace.value().get_number(irCtx)
		                                                           : irCtx->dataLayout.getProgramAddressSpace());
	}
	allPtrTypes.push_back(this);
}

PtrType* PtrType::get(bool isSubtypeVariable, Type* type, bool nonNullable, Locality locality, bool hasMulti,
                      Maybe<AddressSpace> addressSpace, ir::Ctx* irCtx) {
	for (auto* typ : allPtrTypes) {
		if (typ->get_subtype()->is_same(type) && (typ->is_subtype_variable() == isSubtypeVariable) &&
		    typ->get_locality().is_same(locality) && (typ->is_multi() == hasMulti) &&
		    (typ->nonNullable == nonNullable) && ir::AddressSpace::compare(typ->get_address_space(), addressSpace)) {
			return typ;
		}
	}
	return std::construct_at(OwnNormal(PtrType), isSubtypeVariable, type, nonNullable, locality, hasMulti,
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

Locality PtrType::get_locality() const { return locality; }

u32 PtrType::usable_address_space(ir::Ctx* irCtx) const {
	if (addressSpace.has_value()) {
		return addressSpace->get_number(irCtx);
	} else {
		return irCtx->dataLayout.getProgramAddressSpace();
	}
}

TypeKind PtrType::type_kind() const { return TypeKind::POINTER; }

String PtrType::to_string() const {
	return String(is_multi() ? (nonNullable ? "multi![" : "multi:[") : (nonNullable ? "ptr![" : "ptr:[")) +
	       String(is_subtype_variable() ? "var " : "") + subType->to_string() + (locality.is_none() ? "" : " ") +
	       locality.to_string() + "]";
}

} // namespace qat::ir
