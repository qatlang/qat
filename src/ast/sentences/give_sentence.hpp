#ifndef QAT_AST_SENTENCES_GIVE_SENTENCE_HPP
#define QAT_AST_SENTENCES_GIVE_SENTENCE_HPP

#include "../expression.hpp"
#include "../sentence.hpp"
#include <optional>

namespace qat::ast {
/**
 *  GiveSentence represents a return statement in the language
 *
 */
class GiveSentence : public Sentence {
private:
  /**
   *  Expression that should be returned/given
   *
   */
  std::optional<Expression *> give_expr;

public:
  /**
   *  GiveSentence represents a return statement in the language
   *
   * @param _fileRange
   */
  GiveSentence(std::optional<Expression *> _given_expr,
               utils::FileRange _fileRange);

  IR::Value *emit(IR::Context *ctx);

  nuo::Json toJson() const;

  NodeType nodeType() const { return NodeType::giveSentence; }
};

} // namespace qat::ast

#endif