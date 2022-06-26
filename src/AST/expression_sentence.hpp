#ifndef QAT_EXPRESSION_SENTENCE_HPP
#define QAT_EXPRESSION_SENTENCE_HPP

#include "./expression.hpp"
#include "./sentence.hpp"

namespace qat {
namespace AST {

/**
 *  ExpressionSentence is used to represent a sentence that is a
 * standalone expression. Like a function call with no assignment,  lonely
 * binary or unary expression or series of function calls...
 *
 */
class ExpressionSentence : public Sentence {
  /**
   *  Expression in this sentence
   *
   */
  Expression *expr;

public:
  /**
   *  Construct a new Sentence that is just an expression
   *
   * @param _exp Expression in the sentence
   * @param _filePlacement
   */
  ExpressionSentence(Expression *_expr, utils::FilePlacement _filePlacement);

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
  NodeType nodeType() { return NodeType::expressionSentence; }
};

} // namespace AST
} // namespace qat

#endif