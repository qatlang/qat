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

#ifndef QAT_AST_TYPES_INT_HPP
#define QAT_AST_TYPES_INT_HPP

#include "../../IR/generator.hpp"
#include "qat_type.hpp"
#include "llvm/IR/Type.h"

namespace qat {
namespace AST {

/**
 * @brief Integer datatype in the language
 *
 */
class IntegerType : public QatType {
private:
  /**
   * @brief Bitwidth of the integer
   *
   */
  const unsigned int bitWidth;

public:
  /**
   * @brief Construct an Integer type with a bitwidth
   *
   * @param _bitWidth Bitwidth of the integer type
   * @param _filePlacement
   */
  IntegerType(const unsigned int _bitWidth,
              const utils::FilePlacement _filePlacement);

  /**
   * @brief This is the code generator function that handles the generation of
   * LLVM IR
   *
   * @param generator The IR::Generator instance that handles LLVM IR Generation
   * @return llvm::Type*
   */
  llvm::Type *generate(IR::Generator *generator);

  /**
   * @brief TypeKind is used to detect variants of the QatType
   *
   * @return TypeKind
   */
  TypeKind typeKind();

  /**
   * @brief Whether the provided integer is the bitwidth of the IntegerType
   *
   * @param width Bitwidth to check for
   * @return true If the width matches
   * @return false If the width does not match
   */
  bool isBitWidth(const unsigned int width) const;
};
} // namespace AST
} // namespace qat

#endif