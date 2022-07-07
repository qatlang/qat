#include "./variability.hpp"
#include <llvm/ADT/ArrayRef.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Metadata.h>
#include <llvm/Support/Casting.h>

#define VARIABILITY "variability"
#define CONST "const"
#define VAR "var"

namespace qat {
namespace utils {

bool Variability::get(llvm::Instruction *value) {
  return llvm::dyn_cast<llvm::MDString>(
             value->getMetadata(VARIABILITY)->getOperand(0))
             ->getString() == llvm::StringRef("var");
}

bool Variability::get(llvm::Argument *value) {
  return !(value->hasAttribute(llvm::Attribute::AttrKind::ReadOnly));
}

bool Variability::get(llvm::GlobalVariable *value) {
  return !(value->isConstant());
}

void Variability::set(llvm::LLVMContext &context, llvm::Instruction *value,
                      bool is_var) {
  if (value->hasMetadata(VARIABILITY)) {
    auto mdn = (llvm::MDTuple *)value->getMetadata(VARIABILITY);
    std::vector<llvm::MDString *> vals;
    for (unsigned i = 0; i < mdn->getNumOperands(); i++) {
      vals.push_back(llvm::dyn_cast<llvm::MDString>(mdn->getOperand(i)));
    }
    vals.push_back(llvm::MDString::get(context, is_var ? VAR : CONST));
    value->setMetadata(VARIABILITY, mdn);
  } else {
    value->setMetadata(
        VARIABILITY,
        llvm::MDNode::get(
            context, llvm::MDString::get(context, (is_var ? VAR : CONST))));
  }
}

void Variability::set(llvm::Argument *value, bool is_var) {
  if (!is_var) {
    value->addAttr(llvm::Attribute::AttrKind::ReadOnly);
  }
}

void Variability::set(llvm::LLVMContext &ctx, llvm::Function *fn, bool is_var) {
  fn->setMetadata(
      VARIABILITY,
      llvm::MDNode::get(ctx, llvm::MDString::get(ctx, (is_var ? VAR : CONST))));
}

void Variability::propagate(llvm::LLVMContext &context,
                            llvm::Instruction *source,
                            llvm::Instruction *target) {
  auto source_md = source->getMetadata(VARIABILITY);
  target->setMetadata(
      VARIABILITY,
      llvm::MDNode::get(context,
                        llvm::MDString::get(
                            context, source_md ? llvm::dyn_cast<llvm::MDString>(
                                                     source_md->getOperand(0))
                                                     ->getString()
                                                     .str()
                                               : CONST)));
}

void Variability::propagate(llvm::Argument *source, llvm::Argument *target) {
  if (source->hasAttribute(llvm::Attribute::AttrKind::ReadOnly)) {
    target->addAttr(llvm::Attribute::AttrKind::ReadOnly);
  }
}

void Variability::propagate(llvm::Instruction *source, llvm::Argument *target) {
  auto source_md = source->getMetadata(VARIABILITY);
  if (source_md) {
    if (llvm::dyn_cast<llvm::MDString>(source_md->getOperand(0))->getString() !=
        llvm::StringRef(CONST)) {
      target->addAttr(llvm::Attribute::AttrKind::ReadOnly);
    }
  }
}

void Variability::propagate(llvm::LLVMContext &context, llvm::Argument *source,
                            llvm::Instruction *target) {
  target->setMetadata(
      VARIABILITY,
      llvm::MDNode::get(
          context,
          llvm::MDString::get(context, (source->hasAttribute(
                                            llvm::Attribute::AttrKind::ReadOnly)
                                            ? CONST
                                            : VAR))));
}

void Variability::propagate(llvm::LLVMContext &context,
                            llvm::GlobalVariable *source,
                            llvm::Instruction *target) {
  target->setMetadata(
      VARIABILITY,
      llvm::MDNode::get(
          context,
          llvm::MDString::get(context, (source->isConstant() ? CONST : VAR))));
}

} // namespace utils
} // namespace qat