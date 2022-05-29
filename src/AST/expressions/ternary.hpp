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

#ifndef QAT_AST_IF_ELSE_HPP
#define QAT_AST_IF_ELSE_HPP

#include "../../utils/llvm_type_to_name.hpp"
#include "../expression.hpp"
#include "../node_type.hpp"
#include "../sentence.hpp"
#include "llvm/Support/SHA256.h"
#include <vector>

namespace qat {
namespace AST {

/**
 * @brief TernaryExpression represents an expression created by the ternary
 * operator. This requires 3 expressions. The first one is the condition on
 * which the operation takes place. If the condition is true, the second
 * expression is the result. If the condition is false, the third expression is
 * the result
 *
 */
class TernaryExpression : public Expression {
private:
  /**
   * @brief The condition that determines the result of this expression
   *
   */
  Expression condition;

  /**
   * @brief The expression to be used if the provided condition is true
   *
   */
  Expression if_expr;

  /**
   * @brief The expression to be used if the provided condition is false
   *
   */
  Expression else_expr;

public:
  /**
   * @brief Construct a new Ternary Expression object.
   *
   * This represents a ternary expression in the language
   *
   * @param _condition Condition that determines the result of the operation
   * @param _ifExpression Expression returned if the condition is true
   * @param _elseExpression Expression returned if the condition is false
   * @param _filePlacement
   */
  TernaryExpression(Expression _condition, Expression _ifExpression,
                    Expression _elseExpression,
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
  NodeType nodeType() { return NodeType::ternaryExpression; }
};
} // namespace AST
} // namespace qat

#endif