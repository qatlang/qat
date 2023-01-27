#include "./opaque.hpp"
#include "../context.hpp"
#include "../qat_module.hpp"
#include "llvm/IR/DerivedTypes.h"

namespace qat::IR {

OpaqueType::OpaqueType(Identifier _name, Maybe<String> _genericVariantName, IR::QatModule* _parent, Maybe<usize> _size,
                       llvm::LLVMContext& ctx)
    : name(std::move(_name)), genericVariantName(std::move(_genericVariantName)), parent(_parent), size(_size) {
  llvmType = llvm::StructType::create(ctx, parent->getFullNameWithChild(name.value));
}

OpaqueType* OpaqueType::get(Identifier name, Maybe<String> genericVariantName, IR::QatModule* parent, Maybe<usize> size,
                            llvm::LLVMContext& llCtx) {
  return new OpaqueType(std::move(name), std::move(genericVariantName), parent, size, llCtx);
}

String OpaqueType::getFullName() const { return parent->getFullNameWithChild(name.value); }

Identifier OpaqueType::getName() const { return name; }

bool OpaqueType::hasSubType() const { return subTy != nullptr; }

void OpaqueType::setSubType(IR::ExpandedType* _subTy) {
  subTy    = _subTy;
  llvmType = subTy->getLLVMType();
}

IR::QatType* OpaqueType::getSubType() const { return subTy; }

bool OpaqueType::hasSize() const { return size.has_value(); }

bool OpaqueType::isDestructible() const { return (subTy && subTy->isDestructible()) || (destructor != nullptr); }

void OpaqueType::destroyValue(IR::Context* ctx, Vec<IR::Value*> vals, IR::Function* fun) {
  if (subTy) {
    subTy->destroyValue(ctx, vals, fun);
  } else if (destructor && !vals.empty()) {
    if (vals.front()->isReference()) {
      vals.front()->loadImplicitPointer(ctx->builder);
    }
    (void)destructor->call(ctx, {vals.front()->getLLVM()}, ctx->getMod());
  }
}

TypeKind OpaqueType::typeKind() const { return TypeKind::opaque; }

String OpaqueType::toString() const { return getFullName(); }

} // namespace qat::IR