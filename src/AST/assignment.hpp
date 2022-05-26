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

#ifndef QAT_AST_ASSIGNMENT_HPP
#define QAT_AST_ASSIGNMENT_HPP

#include "../IR/generator.hpp"
#include "../utils/variability.hpp"
#include "./expression.hpp"
#include "./node_type.hpp"
#include "./sentence.hpp"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include <string>

namespace qat {
namespace AST {
/**
 * @brief Assignment represents storage of a value to a variable in the
 * language. This will throw errors if the entity to be reassigned is not a
 * variable.
 */
class Assignment : public Sentence {
private:
  /**
   * @brief Name of the variable to which an expression has to be assigned
   *
   */
  std::string name;

  /**
   * @brief Expression instance that represents the expression that has to be
   * assigned to the variable
   *
   */
  Expression value;

public:
  /**
   * @brief Assignment represents storage of a value to a variable in the
   * language. This will throw errors if the entity to be reassigned is not a
   * variable
   *
   * @param _name Name of the variable that has to be reassigned
   * @param _value The Expression instance that represents the expression that
   * should be assigned to the variable
   * @param _filePlacement FilePLacement instance that represents the range
   * spanned by the tokens making up this AST member
   */
  Assignment(std::string _name, Expression _value,
             utils::FilePlacement _filePlacement);

  /**
   * @brief This is the code generator function that handles the generation of
   * LLVM IR
   *
   * @param generator The IR::Generator instance that handles LLVM IR Generation
   * @return llvm::Value*
   */
  llvm::Value *generate(IR::Generator *generator);

  /**
   * @brief Type of the node represented by this AST member
   *
   * @return NodeType
   */
  NodeType nodeType() { return qat::AST::NodeType::reassignment; }
};
} // namespace AST
} // namespace qat

#endif