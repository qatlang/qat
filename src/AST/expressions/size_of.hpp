#ifndef QAT_AST_EXPRESSIONS_SIZE_OF_HPP
#define QAT_AST_EXPRESSIONS_SIZE_OF_HPP

#include "../expression.hpp"
#include "llvm/IR/Constants.h"

namespace qat {
namespace AST {

/**
 * @brief SizeOf is used to find the size of the type of the provided Expression
 *
 */
class SizeOf : public Expression {
private:
  /**
   * @brief Expression for which the size of the type has to be calculated
   *
   */
  Expression expression;

public:
  /**
   * @brief SizeOf is used to find the size of the type of the provided
   * Expression
   *
   * @param _expression Expression to be used to find the size of the type
   * @param _filePlacement
   */
  SizeOf(Expression _expression, utils::FilePlacement _filePlacement);

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
  NodeType nodeType() { return NodeType::sizeOf; }
};

} // namespace AST
} // namespace qat

#endif