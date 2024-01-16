#ifndef QAT_AST_SENTENCES_EXPRESSION_SENTENCE_HPP
#define QAT_AST_SENTENCES_EXPRESSION_SENTENCE_HPP

#include "../expression.hpp"
#include "../sentence.hpp"

namespace qat::ast {

class ExpressionSentence : public Sentence {
  Expression* expr;

public:
  ExpressionSentence(Expression* _expr, FileRange _fileRange) : Sentence(_fileRange), expr(_expr) {}

  useit static inline ExpressionSentence* create(Expression* _expr, FileRange _fileRange) {
    return std::construct_at(OwnNormal(ExpressionSentence), _expr, _fileRange);
  }

  useit IR::Value* emit(IR::Context* ctx);
  useit Json       toJson() const;
  useit NodeType   nodeType() const { return NodeType::EXPRESSION_SENTENCE; }
};

} // namespace qat::ast

#endif