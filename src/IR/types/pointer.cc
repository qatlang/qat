#include "./pointer.hpp"
#include "../function.hpp"
#include "region.hpp"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"

namespace qat::ir {

PointerOwner PointerOwner::OfHeap() { return PointerOwner{.owner = nullptr, .ownerTy = PointerOwnerType::heap}; }

PointerOwner PointerOwner::OfType(Type* type) {
  return PointerOwner{.owner = (void*)type, .ownerTy = PointerOwnerType::type};
}

PointerOwner PointerOwner::OfAnonymous() {
  return PointerOwner{.owner = nullptr, .ownerTy = PointerOwnerType::anonymous};
}

PointerOwner PointerOwner::OfParentFunction(Function* fun) {
  return PointerOwner{.owner = (void*)fun, .ownerTy = PointerOwnerType::parentFunction};
}

PointerOwner PointerOwner::OfParentInstance(Type* typ) {
  return PointerOwner{.owner = (void*)typ, .ownerTy = PointerOwnerType::parentInstance};
}

PointerOwner PointerOwner::OfRegion(Region* region) {
  return PointerOwner{.owner = region, .ownerTy = PointerOwnerType::region};
}

PointerOwner PointerOwner::OfAnyRegion() {
  return PointerOwner{.owner = nullptr, .ownerTy = PointerOwnerType::anyRegion};
}

bool PointerOwner::is_same(const PointerOwner& other) const {
  if (ownerTy == other.ownerTy) {
    switch (ownerTy) {
      case PointerOwnerType::anonymous:
      case PointerOwnerType::Static:
      case PointerOwnerType::heap:
      case PointerOwnerType::anyRegion:
        return true;
      case PointerOwnerType::region:
        return ownerAsRegion()->is_same(other.ownerAsRegion());
      case PointerOwnerType::type:
        return ownerAsType()->is_same(other.ownerAsType());
      case PointerOwnerType::parentFunction:
        return owner_as_parent_function()->get_id() == other.owner_as_parent_function()->get_id();
      case PointerOwnerType::parentInstance:
        return ownerAsParentType()->get_id() == other.ownerAsParentType()->get_id();
    }
  } else {
    return false;
  }
}

String PointerOwner::to_string() const {
  switch (ownerTy) {
    case PointerOwnerType::anyRegion:
      return "region";
    case PointerOwnerType::region:
      return "region(" + ownerAsRegion()->to_string() + ")";
    case PointerOwnerType::heap:
      return "heap";
    case PointerOwnerType::anonymous:
      return "";
    case PointerOwnerType::type:
      return "type(" + ownerAsType()->to_string() + ")";
    case PointerOwnerType::parentInstance:
      return "''(" + ownerAsParentType()->to_string() + ")";
    case PointerOwnerType::parentFunction:
      return "own(" + owner_as_parent_function()->get_full_name() + ")";
    case PointerOwnerType::Static:
      return "static";
  }
}

PointerType::PointerType(bool _isSubtypeVariable, Type* _type, bool _nonNullable, PointerOwner _owner, bool _hasMulti,
                         ir::Ctx* irCtx)
    : subType(_type), isSubtypeVar(_isSubtypeVariable), owner(_owner), hasMulti(_hasMulti), nonNullable(_nonNullable) {
  if (_hasMulti) {
    linkingName = (nonNullable ? "qat'multiptr![" : "qat'multiptr:[") + String(isSubtypeVar ? "var " : "") +
                  subType->get_name_for_linking() + (owner.isAnonymous() ? "" : ",") + owner.to_string() + "]";
    if (llvm::StructType::getTypeByName(irCtx->llctx, linkingName)) {
      llvmType = llvm::StructType::getTypeByName(irCtx->llctx, linkingName);
    } else {
      llvmType = llvm::StructType::create(
          {llvm::PointerType::get(llvm::Type::getInt8Ty(irCtx->llctx), irCtx->dataLayout->getProgramAddressSpace()),
           llvm::Type::getIntNTy(irCtx->llctx,
                                 irCtx->clangTargetInfo->getTypeWidth(irCtx->clangTargetInfo->getSizeType()))},
          linkingName);
    }
  } else {
    linkingName = (nonNullable ? "qat'ptr![" : "qat'ptr:[") + String(isSubtypeVar ? "var " : "") +
                  subType->get_name_for_linking() + (owner.isAnonymous() ? "" : ",") + owner.to_string() + "]";
    llvmType = llvm::PointerType::get(llvm::Type::getInt8Ty(irCtx->llctx), irCtx->dataLayout->getProgramAddressSpace());
  }
}

PointerType* PointerType::get(bool _isSubtypeVariable, Type* _type, bool _nonNullable, PointerOwner _owner,
                              bool _hasMulti, ir::Ctx* irCtx) {
  for (auto* typ : allTypes) {
    if (typ->is_pointer()) {
      if (typ->as_pointer()->get_subtype()->is_same(_type) &&
          (typ->as_pointer()->isSubtypeVariable() == _isSubtypeVariable) &&
          typ->as_pointer()->getOwner().is_same(_owner) && (typ->as_pointer()->isMulti() == _hasMulti) &&
          (typ->as_pointer()->nonNullable == _nonNullable)) {
        return typ->as_pointer();
      }
    }
  }
  return new PointerType(_isSubtypeVariable, _type, _nonNullable, _owner, _hasMulti, irCtx);
}

bool PointerType::isSubtypeVariable() const { return isSubtypeVar; }

bool PointerType::is_type_sized() const { return true; }

bool PointerType::has_prerun_default_value() const { return !nonNullable; }

PrerunValue* PointerType::get_prerun_default_value(ir::Ctx* irCtx) {
  if (has_prerun_default_value()) {
    if (isMulti()) {
      return ir::PrerunValue::get(
          llvm::ConstantStruct::get(
              llvm::cast<llvm::StructType>(get_llvm_type()),
              {llvm::ConstantPointerNull::get(llvm::PointerType::get(
                  get_subtype()->is_void() ? llvm::Type::getInt8Ty(irCtx->llctx) : get_subtype()->get_llvm_type(),
                  irCtx->dataLayout.value().getProgramAddressSpace()))}),
          this);
    } else {
      return ir::PrerunValue::get(llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(get_llvm_type())), this);
    }
  } else {
    irCtx->Error("Type " + irCtx->color(to_string()) + " do not have a default value", None);
    return nullptr;
  }
}

bool PointerType::is_trivially_copyable() const { return true; }

bool PointerType::is_trivially_movable() const { return !nonNullable; }

bool PointerType::isMulti() const { return hasMulti; }

bool PointerType::isNullable() const { return !nonNullable; }

bool PointerType::isNonNullable() const { return nonNullable; }

Type* PointerType::get_subtype() const { return subType; }

PointerOwner PointerType::getOwner() const { return owner; }

TypeKind PointerType::type_kind() const { return TypeKind::pointer; }

String PointerType::to_string() const {
  return String(isMulti() ? (nonNullable ? "multiptr![" : "multiptr:[") : (nonNullable ? "ptr![" : "ptr:[")) +
         String(isSubtypeVariable() ? "var " : "") + subType->to_string() + (owner.isAnonymous() ? "" : " ") +
         owner.to_string() + "]";
}

} // namespace qat::ir
