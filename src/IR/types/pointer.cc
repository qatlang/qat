#include "./pointer.hpp"
#include "../function.hpp"
#include "region.hpp"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"

namespace qat::ir {

MarkOwner MarkOwner::OfHeap() { return MarkOwner{.owner = nullptr, .ownerTy = PointerOwnerType::heap}; }

MarkOwner MarkOwner::OfType(Type* type) { return MarkOwner{.owner = (void*)type, .ownerTy = PointerOwnerType::type}; }

MarkOwner MarkOwner::OfAnonymous() { return MarkOwner{.owner = nullptr, .ownerTy = PointerOwnerType::anonymous}; }

MarkOwner MarkOwner::OfParentFunction(Function* fun) {
  return MarkOwner{.owner = (void*)fun, .ownerTy = PointerOwnerType::parentFunction};
}

MarkOwner MarkOwner::OfParentInstance(Type* typ) {
  return MarkOwner{.owner = (void*)typ, .ownerTy = PointerOwnerType::parentInstance};
}

MarkOwner MarkOwner::OfRegion(Region* region) {
  return MarkOwner{.owner = region, .ownerTy = PointerOwnerType::region};
}

MarkOwner MarkOwner::OfAnyRegion() { return MarkOwner{.owner = nullptr, .ownerTy = PointerOwnerType::anyRegion}; }

bool MarkOwner::is_same(const MarkOwner& other) const {
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

String MarkOwner::to_string() const {
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

MarkType::MarkType(bool _isSubtypeVariable, Type* _type, bool _nonNullable, MarkOwner _owner, bool _hasMulti,
                   ir::Ctx* irCtx)
    : subType(_type), isSubtypeVar(_isSubtypeVariable), owner(_owner), hasMulti(_hasMulti), nonNullable(_nonNullable) {
  if (_hasMulti) {
    linkingName = (nonNullable ? "qat'slice![" : "qat'slice:[") + String(isSubtypeVar ? "var " : "") +
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
    linkingName = (nonNullable ? "qat'mark![" : "qat'mark:[") + String(isSubtypeVar ? "var " : "") +
                  subType->get_name_for_linking() + (owner.isAnonymous() ? "" : ",") + owner.to_string() + "]";
    llvmType = llvm::PointerType::get(llvm::Type::getInt8Ty(irCtx->llctx), irCtx->dataLayout->getProgramAddressSpace());
  }
}

MarkType* MarkType::get(bool _isSubtypeVariable, Type* _type, bool _nonNullable, MarkOwner _owner, bool _hasMulti,
                        ir::Ctx* irCtx) {
  for (auto* typ : allTypes) {
    if (typ->is_mark()) {
      if (typ->as_mark()->get_subtype()->is_same(_type) &&
          (typ->as_mark()->isSubtypeVariable() == _isSubtypeVariable) && typ->as_mark()->getOwner().is_same(_owner) &&
          (typ->as_mark()->isSlice() == _hasMulti) && (typ->as_mark()->nonNullable == _nonNullable)) {
        return typ->as_mark();
      }
    }
  }
  return new MarkType(_isSubtypeVariable, _type, _nonNullable, _owner, _hasMulti, irCtx);
}

bool MarkType::isSubtypeVariable() const { return isSubtypeVar; }

bool MarkType::is_type_sized() const { return true; }

bool MarkType::has_prerun_default_value() const { return !nonNullable; }

PrerunValue* MarkType::get_prerun_default_value(ir::Ctx* irCtx) {
  if (has_prerun_default_value()) {
    if (isSlice()) {
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

bool MarkType::is_trivially_copyable() const { return true; }

bool MarkType::is_trivially_movable() const { return !nonNullable; }

bool MarkType::isSlice() const { return hasMulti; }

bool MarkType::isNullable() const { return !nonNullable; }

bool MarkType::isNonNullable() const { return nonNullable; }

Type* MarkType::get_subtype() const { return subType; }

MarkOwner MarkType::getOwner() const { return owner; }

TypeKind MarkType::type_kind() const { return TypeKind::pointer; }

String MarkType::to_string() const {
  return String(isSlice() ? (nonNullable ? "slice![" : "slice:[") : (nonNullable ? "mark![" : "mark:[")) +
         String(isSubtypeVariable() ? "var " : "") + subType->to_string() + (owner.isAnonymous() ? "" : " ") +
         owner.to_string() + "]";
}

} // namespace qat::ir
