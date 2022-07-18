#ifndef QAT_AST_SENTENCES_SAY_HPP
#define QAT_AST_SENTENCES_SAY_HPP

#include "../expression.hpp"
#include "../sentence.hpp"

namespace qat::ast {

class Say : public Sentence {
private:
  std::vector<Expression *> expressions;

public:
  Say(std::vector<Expression *> _expressions, utils::FileRange _fileRange)
      : expressions(_expressions), Sentence(_fileRange) {}

  IR::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  NodeType nodeType() const { return NodeType::saySentence; }

  nuo::Json toJson() const;
};

} // namespace qat::ast

#endif