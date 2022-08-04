#ifndef QAT_AST_SENTENCES_SAY_HPP
#define QAT_AST_SENTENCES_SAY_HPP

#include "../expression.hpp"
#include "../sentence.hpp"

namespace qat::ast {

class Say : public Sentence {
private:
  Vec<Expression *> expressions;

public:
  Say(Vec<Expression *> _expressions, utils::FileRange _fileRange)
      : expressions(_expressions), Sentence(_fileRange) {}

  IR::Value *emit(IR::Context *ctx);

  NodeType nodeType() const { return NodeType::saySentence; }

  nuo::Json toJson() const;
};

} // namespace qat::ast

#endif