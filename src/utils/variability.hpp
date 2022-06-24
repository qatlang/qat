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

#ifndef QAT_UTILS_VARIABILITY_HPP
#define QAT_UTILS_VARIABILITY_HPP

#include "llvm/IR/Argument.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Metadata.h"

namespace qat {
namespace utils {
/**
 * @brief Variability is a static class that hosts the functions
 * associated with the metadata that handles variability of values
 *
 */
class Variability {
public:
  static bool get(llvm::Instruction *value);
  static bool get(llvm::Argument *value);
  static bool get(llvm::GlobalVariable *value);
  static void set(llvm::LLVMContext &context, llvm::Instruction *value,
                  bool is_var);
  static void set(llvm::Argument *value, bool is_var);
  static void propagate(llvm::LLVMContext &context, llvm::Instruction *source,
                        llvm::Instruction *target);
  static void propagate(llvm::Argument *source, llvm::Argument *target);
  static void propagate(llvm::Instruction *source, llvm::Argument *target);
  static void propagate(llvm::LLVMContext &context, llvm::Argument *source,
                        llvm::Instruction *target);
  static void propagate(llvm::LLVMContext &context,
                        llvm::GlobalVariable *source,
                        llvm::Instruction *target);
};
} // namespace utils
} // namespace qat

#endif