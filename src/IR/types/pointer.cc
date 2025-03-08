#include "./pointer.hpp"
#include "../function.hpp"
#include "./region.hpp"

#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>

namespace qat::ir {

MarkOwner MarkOwner::of_heap() { return MarkOwner{.owner = nullptr, .ownerTy = PointerOwnerType::heap}; }

MarkOwner MarkOwner::of_type(Type* type) { return MarkOwner{.owner = (void*)type, .ownerTy = PointerOwnerType::type}; }

MarkOwner MarkOwner::of_anonymous() { return MarkOwner{.owner = nullptr, .ownerTy = PointerOwnerType::anonymous}; }

MarkOwner MarkOwner::of_parent_function(Function* fun) {
	return MarkOwner{.owner = (void*)fun, .ownerTy = PointerOwnerType::parentFunction};
}

MarkOwner MarkOwner::of_parent_instance(Type* typ) {
	return MarkOwner{.owner = (void*)typ, .ownerTy = PointerOwnerType::parentInstance};
}

MarkOwner MarkOwner::of_region(Region* region) {
	return MarkOwner{.owner = region, .ownerTy = PointerOwnerType::region};
}

MarkOwner MarkOwner::of_any_region() { return MarkOwner{.owner = nullptr, .ownerTy = PointerOwnerType::anyRegion}; }

bool MarkOwner::is_same(const MarkOwner& other) const {
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

String MarkOwner::to_string() const {
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

MarkType::MarkType(bool _isSubtypeVariable, Type* _type, bool _nonNullable, MarkOwner _owner, bool _hasMulti,
                   ir::Ctx* irCtx)
    : subType(_type), isSubtypeVar(_isSubtypeVariable), owner(_owner), hasMulti(_hasMulti), nonNullable(_nonNullable) {
	if (_hasMulti) {
		linkingName = (nonNullable ? "qat'slice![" : "qat'slice:[") + String(isSubtypeVar ? "var " : "") +
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
		linkingName = (nonNullable ? "qat'mark![" : "qat'mark:[") + String(isSubtypeVar ? "var " : "") +
		              subType->get_name_for_linking() + (owner.is_of_anonymous() ? "" : ",") + owner.to_string() + "]";
		llvmType =
		    llvm::PointerType::get(llvm::Type::getInt8Ty(irCtx->llctx), irCtx->dataLayout.getProgramAddressSpace());
	}
}

MarkType* MarkType::get(bool _isSubtypeVariable, Type* _type, bool _nonNullable, MarkOwner _owner, bool _hasMulti,
                        ir::Ctx* irCtx) {
	for (auto* typ : allTypes) {
		if (typ->is_mark()) {
			if (typ->as_mark()->get_subtype()->is_same(_type) &&
			    (typ->as_mark()->is_subtype_variable() == _isSubtypeVariable) &&
			    typ->as_mark()->get_owner().is_same(_owner) && (typ->as_mark()->is_slice() == _hasMulti) &&
			    (typ->as_mark()->nonNullable == _nonNullable)) {
				return typ->as_mark();
			}
		}
	}
	return std::construct_at(OwnNormal(MarkType), _isSubtypeVariable, _type, _nonNullable, _owner, _hasMulti, irCtx);
}

bool MarkType::is_subtype_variable() const { return isSubtypeVar; }

bool MarkType::is_type_sized() const { return true; }

bool MarkType::has_prerun_default_value() const { return not nonNullable; }

PrerunValue* MarkType::get_prerun_default_value(ir::Ctx* irCtx) {
	if (has_prerun_default_value()) {
		if (is_slice()) {
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

bool MarkType::is_trivially_copyable() const { return true; }

bool MarkType::is_trivially_movable() const { return not nonNullable; }

bool MarkType::is_slice() const { return hasMulti; }

bool MarkType::is_nullable() const { return not nonNullable; }

bool MarkType::is_non_nullable() const { return nonNullable; }

Type* MarkType::get_subtype() const { return subType; }

MarkOwner MarkType::get_owner() const { return owner; }

TypeKind MarkType::type_kind() const { return TypeKind::MARK; }

String MarkType::to_string() const {
	return String(is_slice() ? (nonNullable ? "slice![" : "slice:[") : (nonNullable ? "mark![" : "mark:[")) +
	       String(is_subtype_variable() ? "var " : "") + subType->to_string() + (owner.is_of_anonymous() ? "" : " ") +
	       owner.to_string() + "]";
}

} // namespace qat::ir
