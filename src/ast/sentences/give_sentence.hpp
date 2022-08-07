#ifndef QAT_AST_SENTENCES_GIVE_SENTENCE_HPP
#define QAT_AST_SENTENCES_GIVE_SENTENCE_HPP

#include "../expression.hpp"
#include "../sentence.hpp"
#include <optional>

namespace qat::ast {

class GiveSentence : public Sentence {
private:
  Maybe<Expression *> give_expr;

public:
  GiveSentence(Maybe<Expression *> _given_expr, utils::FileRange _fileRange);

  useit IR::Value *emit(IR::Context *ctx) override;
  useit nuo::Json toJson() const override;
  useit NodeType  nodeType() const override { return NodeType::giveSentence; }
};

} // namespace qat::ast

#endif