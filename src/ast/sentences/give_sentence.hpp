#ifndef QAT_AST_SENTENCES_GIVE_SENTENCE_HPP
#define QAT_AST_SENTENCES_GIVE_SENTENCE_HPP

#include "../expression.hpp"
#include "../sentence.hpp"
#include <optional>

namespace qat::ast {

class GiveSentence : public Sentence {
private:
  Maybe<Expression*> give_expr;

public:
  GiveSentence(Maybe<Expression*> _given_expr, FileRange _fileRange)
      : Sentence(std::move(_fileRange)), give_expr(_given_expr) {}

  useit static inline GiveSentence* create(Maybe<Expression*> _given_expr, FileRange _fileRange) {
    return std::construct_at(OwnNormal(GiveSentence), _given_expr, _fileRange);
  }

  useit IR::Value* emit(IR::Context* ctx) override;
  useit Json       toJson() const override;
  useit NodeType   nodeType() const override { return NodeType::GIVE_SENTENCE; }
};

} // namespace qat::ast

#endif