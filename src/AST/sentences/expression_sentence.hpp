#ifndef QAT_AST_SENTENCES_EXPRESSION_SENTENCE_HPP
#define QAT_AST_SENTENCES_EXPRESSION_SENTENCE_HPP

#include "../expression.hpp"
#include "../sentence.hpp"

namespace qat::AST {

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

  IR::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  nuo::Json toJson() const;

  NodeType nodeType() const { return NodeType::expressionSentence; }
};

} // namespace qat::AST

#endif