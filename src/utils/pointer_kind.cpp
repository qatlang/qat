#include "./pointer_kind.hpp"

#define POINTER_KIND "pointer_kind"
#define REFERENCE "reference"
#define POINTER "pointer"

namespace qat {
namespace utils {

void PointerKind::set(llvm::LLVMContext &ctx, llvm::AllocaInst *inst,
                      bool is_reference) {
  if (inst->getAllocatedType()->isPointerTy()) {
    inst->setMetadata(
        POINTER_KIND,
        llvm::MDNode::get(
            ctx, llvm::MDString::get(ctx, is_reference ? REFERENCE : POINTER)));
  }
}

void PointerKind::set(llvm::LLVMContext &ctx, llvm::GlobalVariable *gvar,
                      bool is_reference) {
  if (gvar->getValueType()->isPointerTy()) {
    gvar->setMetadata(
        POINTER_KIND,
        llvm::MDNode::get(
            ctx, llvm::MDString::get(ctx, is_reference ? REFERENCE : POINTER)));
  }
}

void PointerKind::set(llvm::Argument *arg, bool is_reference) {
  if (is_reference) {
    arg->addAttr(llvm::Attribute::AttrKind::Dereferenceable);
  }
}

bool PointerKind::is_reference(llvm::AllocaInst *inst) {
  if (inst->getAllocatedType()->isPointerTy()) {
    return (llvm::dyn_cast<llvm::MDString>(
                inst->getMetadata(POINTER_KIND)->getOperand(0))
                ->getString()
                .str() == REFERENCE);
  } else {
    return false;
  }
}

bool PointerKind::is_reference(llvm::GlobalVariable *gvar) {
  if (gvar->getValueType()->isPointerTy()) {
    return (llvm::dyn_cast<llvm::MDString>(
                gvar->getMetadata(POINTER_KIND)->getOperand(0))
                ->getString()
                .str() == REFERENCE);
  } else {
    return false;
  }
}

bool PointerKind::is_reference(llvm::Argument *arg) {
  return arg->hasAttribute(llvm::Attribute::AttrKind::Dereferenceable);
}

} // namespace utils
} // namespace qat