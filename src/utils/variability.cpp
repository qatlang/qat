/**
 * Qat Programming Language : Copyright 2022 : Aldrin Mathew
 *
 * AAF INSPECTABLE LICENCE - 1.0
 *
 * This project is licensed under the AAF Inspectable Licence 1.0.
 * You are allowed to inspect the source of this project(s) free of
 * cost, and also to verify the authenticity of the product.
 *
 * Unless required by applicable law, this project is provided
 * "AS IS", WITHOUT ANY WARRANTIES OR PROMISES OF ANY KIND, either
 * expressed or implied. The author(s) of this project is not
 * liable for any harms, errors or troubles caused by using the
 * source or the product, unless implied by law. By using this
 * project, or part of it, you are acknowledging the complete terms
 * and conditions of licensing of this project as specified in AAF
 * Inspectable Licence 1.0 available at this URL:
 *
 * https://github.com/aldrinsartfactory/InspectableLicence/
 *
 * This project may contain parts that are not licensed under the
 * same licence. If so, the licences of those parts should be
 * appropriately mentioned in those parts of the project. The
 * Author MAY provide a notice about the parts of the project that
 * are not licensed under the same licence in a publicly visible
 * manner.
 *
 * You are NOT ALLOWED to sell, or distribute THIS project, its
 * contents, the source or the product or the build result of the
 * source under commercial or non-commercial purposes. You are NOT
 * ALLOWED to revamp, rebrand, refactor, modify, the source, product
 * or the contents of this project.
 *
 * You are NOT ALLOWED to use the name, branding and identity of this
 * project to identify or brand any other project. You ARE however
 * allowed to use the name and branding to pinpoint/show the source
 * of the contents/code/logic of any other project. You are not
 * allowed to use the identification of the Authors of this project
 * to associate them to other projects, in a way that is deceiving
 * or misleading or gives out false information.
 */

#include "./variability.hpp"

bool qat::utils::Variability::get(llvm::Instruction *value) {
  return llvm::dyn_cast<llvm::MDString>(
             value->getMetadata(llvm::StringRef("variability"))->getOperand(0))
             ->getString() == llvm::StringRef("var");
}

bool qat::utils::Variability::get(llvm::Argument *value) {
  return !(value->hasAttribute(llvm::Attribute::AttrKind::ImmArg));
}

void qat::utils::Variability::set(llvm::LLVMContext &context,
                                  llvm::Instruction *value, bool is_var) {
  value->setMetadata(
      llvm::StringRef(is_var ? "var" : "const"),
      llvm::MDNode::get(context, llvm::MDString::get(context, "variability")));
}

void qat::utils::Variability::set(llvm::Argument *value, bool is_var) {
  if (!is_var) {
    value->addAttr(llvm::Attribute::AttrKind::ImmArg);
  }
}

void qat::utils::Variability::propagate(llvm::LLVMContext &context,
                                        llvm::Instruction *source,
                                        llvm::Instruction *target) {
  auto source_md = source->getMetadata(llvm::StringRef("variability"));
  target->setMetadata(
      llvm::StringRef(source_md != nullptr ? llvm::dyn_cast<llvm::MDString>(
                                                 source_md->getOperand(0))
                                                 ->getString()
                                                 .str()
                                           : "const"),
      llvm::MDNode::get(context, llvm::MDString::get(context, "variability")));
}

void qat::utils::Variability::propagate(llvm::Argument *source,
                                        llvm::Argument *target) {
  if (source->hasAttribute(llvm::Attribute::AttrKind::ImmArg)) {
    target->addAttr(llvm::Attribute::AttrKind::ImmArg);
  }
}

void qat::utils::Variability::propagate(llvm::Instruction *source,
                                        llvm::Argument *target) {
  auto source_md = source->getMetadata(llvm::StringRef("variability"));
  if (source_md != nullptr) {
    if (llvm::dyn_cast<llvm::MDString>(source_md->getOperand(0))->getString() !=
        llvm::StringRef("var")) {
      target->addAttr(llvm::Attribute::AttrKind::ImmArg);
    }
  }
}

void qat::utils::Variability::propagate(llvm::LLVMContext &context,
                                        llvm::Argument *source,
                                        llvm::Instruction *target) {
  target->setMetadata(
      llvm::StringRef(source->hasAttribute(llvm::Attribute::AttrKind::ImmArg)
                          ? "const"
                          : "var"),
      llvm::MDNode::get(context, llvm::MDString::get(context, "variability")));
}