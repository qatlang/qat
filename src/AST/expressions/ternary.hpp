#ifndef QAT_AST_EXPRESSIONS_TERNARY_HPP
#define QAT_AST_EXPRESSIONS_TERNARY_HPP

#include "../../utils/llvm_type_to_name.hpp"
#include "../../utils/new_block_index.hpp"
#include "../expression.hpp"
#include "../node_type.hpp"
#include "../sentence.hpp"
#include "llvm/Support/SHA256.h"
#include <vector>

namespace qat {
namespace AST {

/**
 *  TernaryExpression represents an expression created by the ternary
 * operator. This requires 3 expressions. The first one is the condition on
 * which the operation takes place. If the condition is true, the second
 * expression is the result. If the condition is false, the third expression is
 * the result
 *
 */
class TernaryExpression : public Expression {
private:
  /**
   *  The condition that determines the result of this expression
   *
   */
  Expression condition;

  /**
   *  The expression to be used if the provided condition is true
   *
   */
  Expression if_expr;

  /**
   *  The expression to be used if the provided condition is false
   *
   */
  Expression else_expr;

public:
  /**
   *  Construct a new Ternary Expression object.
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
   *  This is the code generator function that handles the generation of
   * LLVM IR
   *
   * @param generator The IR::Generator instance that handles LLVM IR Generation
   * @return llvm::Value*
   */
  llvm::Value *generate(IR::Generator *generator);

  /**
   *  Type of the node represented by this AST member
   *
   * @return NodeType
   */
  NodeType nodeType() { return NodeType::ternaryExpression; }
};

} // namespace AST
} // namespace qat

#endif