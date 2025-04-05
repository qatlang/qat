#include "./pointer.hpp"
#include "../function.hpp"
#include "./region.hpp"

#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>

namespace qat::ir {

PtrOwner PtrOwner::of_heap() { return PtrOwner{.owner = nullptr, .ownerTy = PointerOwnerType::heap}; }

PtrOwner PtrOwner::of_type(Type* type) { return PtrOwner{.owner = (void*)type, .ownerTy = PointerOwnerType::type}; }

PtrOwner PtrOwner::of_anonymous() { return PtrOwner{.owner = nullptr, .ownerTy = PointerOwnerType::anonymous}; }

PtrOwner PtrOwner::of_parent_function(Function* fun) {
	return PtrOwner{.owner = (void*)fun, .ownerTy = PointerOwnerType::parentFunction};
}

PtrOwner PtrOwner::of_parent_instance(Type* typ) {
	return PtrOwner{.owner = (void*)typ, .ownerTy = PointerOwnerType::parentInstance};
}

PtrOwner PtrOwner::of_region(Region* region) { return PtrOwner{.owner = region, .ownerTy = PointerOwnerType::region}; }

PtrOwner PtrOwner::of_any_region() { return PtrOwner{.owner = nullptr, .ownerTy = PointerOwnerType::anyRegion}; }

bool PtrOwner::is_same(const PtrOwner& other) const {
	if (ownerTy == other.ownerTy) {
		switch (ownerTy) {
			case PointerOwnerType::anonymous:
			case PointerOwnerType::Static:
			case PointerOwnerType::heap:
			case PointerOwnerType::anyRegion:
				return true;
			case PointerOwnerType::region:
				return owner_as_region()->is_same(other.owner_as_region());
			case PointerOwnerType::type:
				return owner_as_type()->is_same(other.owner_as_type());
			case PointerOwnerType::parentFunction:
				return owner_as_parent_function()->get_id() == other.owner_as_parent_function()->get_id();
			case PointerOwnerType::parentInstance:
				return owner_as_parent_type()->get_id() == other.owner_as_parent_type()->get_id();
		}
	} else {
		return false;
	}
}

String PtrOwner::to_string() const {
	switch (ownerTy) {
		case PointerOwnerType::anyRegion:
			return "region";
		case PointerOwnerType::region:
			return "region(" + owner_as_region()->to_string() + ")";
		case PointerOwnerType::heap:
			return "heap";
		case PointerOwnerType::anonymous:
			return "";
		case PointerOwnerType::type:
			return "type(" + owner_as_type()->to_string() + ")";
		case PointerOwnerType::parentInstance:
			return "''(" + owner_as_parent_type()->to_string() + ")";
		case PointerOwnerType::parentFunction:
			return "own(" + owner_as_parent_function()->get_full_name() + ")";
		case PointerOwnerType::Static:
			return "static";
	}
}

PtrType::PtrType(bool _isSubtypeVariable, Type* _type, bool _nonNullable, PtrOwner _owner, bool _hasMulti,
                 ir::Ctx* irCtx)
    : subType(_type), isSubtypeVar(_isSubtypeVariable), owner(_owner), hasMulti(_hasMulti), nonNullable(_nonNullable) {
	if (_hasMulti) {
		linkingName = (nonNullable ? "qat'multi![" : "qat'multi:[") + String(isSubtypeVar ? "var " : "") +
		              subType->get_name_for_linking() + (owner.is_of_anonymous() ? "" : ",") + owner.to_string() + "]";
		if (llvm::StructType::getTypeByName(irCtx->llctx, linkingName)) {
			llvmType = llvm::StructType::getTypeByName(irCtx->llctx, linkingName);
		} else {
			llvmType = llvm::StructType::create(
			    {llvm::PointerType::get(llvm::Type::getInt8Ty(irCtx->llctx),
			                            irCtx->dataLayout.getProgramAddressSpace()),
			     llvm::Type::getIntNTy(irCtx->llctx,
			                           irCtx->clangTargetInfo->getTypeWidth(irCtx->clangTargetInfo->getSizeType()))},
			    linkingName);
		}
	} else {
		linkingName = (nonNullable ? "qat'ptr![" : "qat'ptr:[") + String(isSubtypeVar ? "var " : "") +
		              subType->get_name_for_linking() + (owner.is_of_anonymous() ? "" : ",") + owner.to_string() + "]";
		llvmType =
		    llvm::PointerType::get(llvm::Type::getInt8Ty(irCtx->llctx), irCtx->dataLayout.getProgramAddressSpace());
	}
}

PtrType* PtrType::get(bool _isSubtypeVariable, Type* _type, bool _nonNullable, PtrOwner _owner, bool _hasMulti,
                      ir::Ctx* irCtx) {
	for (auto* typ : allTypes) {
		if (typ->is_ptr()) {
			if (typ->as_ptr()->get_subtype()->is_same(_type) &&
			    (typ->as_ptr()->is_subtype_variable() == _isSubtypeVariable) &&
			    typ->as_ptr()->get_owner().is_same(_owner) && (typ->as_ptr()->is_multi() == _hasMulti) &&
			    (typ->as_ptr()->nonNullable == _nonNullable)) {
				return typ->as_ptr();
			}
		}
	}
	return std::construct_at(OwnNormal(PtrType), _isSubtypeVariable, _type, _nonNullable, _owner, _hasMulti, irCtx);
}

bool PtrType::is_subtype_variable() const { return isSubtypeVar; }

bool PtrType::is_type_sized() const { return true; }

bool PtrType::has_prerun_default_value() const { return not nonNullable; }

PrerunValue* PtrType::get_prerun_default_value(ir::Ctx* irCtx) {
	if (has_prerun_default_value()) {
		if (is_multi()) {
			return ir::PrerunValue::get(
			    llvm::ConstantStruct::get(
			        llvm::cast<llvm::StructType>(get_llvm_type()),
			        {llvm::ConstantPointerNull::get(llvm::PointerType::get(
			            get_subtype()->is_void() ? llvm::Type::getInt8Ty(irCtx->llctx) : get_subtype()->get_llvm_type(),
			            irCtx->dataLayout.getProgramAddressSpace()))}),
			    this);
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

PtrOwner PtrType::get_owner() const { return owner; }

TypeKind PtrType::type_kind() const { return TypeKind::POINTER; }

String PtrType::to_string() const {
	return String(is_multi() ? (nonNullable ? "multi![" : "multi:[") : (nonNullable ? "ptr![" : "ptr:[")) +
	       String(is_subtype_variable() ? "var " : "") + subType->to_string() + (owner.is_of_anonymous() ? "" : " ") +
	       owner.to_string() + "]";
}

} // namespace qat::ir
