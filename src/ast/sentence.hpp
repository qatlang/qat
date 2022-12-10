#ifndef QAT_AST_SENTENCE_HPP
#define QAT_AST_SENTENCE_HPP

#include "./node.hpp"

namespace qat::ast {

class Sentence : public Node {
public:
  Sentence(FileRange _fileRange) : Node(std::move(_fileRange)) {}

  useit IR::Value* emit(IR::Context* ctx) override = 0;
  useit NodeType   nodeType() const override       = 0;
  useit Json       toJson() const override         = 0;
  ~Sentence() override                             = default;
};

void emitSentences(const Vec<Sentence*>& sentences, IR::Context* ctx);

} // namespace qat::ast

#endif