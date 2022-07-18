#ifndef QAT_AST_SENTENCE_HPP
#define QAT_AST_SENTENCE_HPP

#include "./node.hpp"

namespace qat::ast {

class Sentence : public Node {
public:
  Sentence(utils::FileRange _fileRange) : Node(_fileRange) {}

  virtual IR::Value *emit(IR::Context *ctx) {}

  virtual NodeType nodeType() const {}

  virtual nuo::Json toJson() const {}

  virtual void destroy() {}

  virtual ~Sentence() { this->destroy(); }
};

} // namespace qat::ast

#endif