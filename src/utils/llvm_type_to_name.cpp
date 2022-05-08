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

#include "./llvm_type_to_name.hpp"

std::string qat::utilities::llvmTypeToName(llvm::Type *type) {
  if (type->isStructTy()) {
    auto structType = llvm::dyn_cast<llvm::StructType>(type);
    if (structType->isLiteral()) {
      std::string value = "(";
      for (std::size_t i = 0; i < structType->getNumElements(); i++) {
        value += llvmTypeToName(structType->getElementType(i));
        if (i + 1 < structType->getNumElements()) {
          value += ", ";
        }
      }
      value += ")";
      return value;
    } else {
      return structType->getName().str();
    }
  } else {
    if (type->isIntegerTy()) {
      llvm::IntegerType *integerType = llvm::dyn_cast<llvm::IntegerType>(type);
      return "i" + std::to_string(integerType->getBitWidth());
    } else if (type->isPPC_FP128Ty()) {
      return "f128ppc";
    } else if (type->isFP128Ty()) {
      return "f128";
    } else if (type->isX86_FP80Ty()) {
      return "f80";
    } else if (type->isDoubleTy()) {
      return "f64";
    } else if (type->isFloatTy()) {
      return "f32";
    } else if (type->isVoidTy()) {
      return "void";
    } else if (type->isPointerTy()) {
      llvm::PointerType *pointerType = llvm::dyn_cast<llvm::PointerType>(type);
      if (pointerType->getElementType() != nullptr) {
        return llvmTypeToName(pointerType->getElementType()) + "#";
      } else {
        return "unknown#";
      }
    } else {
      return "unknown";
    }
  }
}