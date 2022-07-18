#ifndef QAT_AST_SENTENCE_HPP
#define QAT_AST_SENTENCE_HPP

#include "./node.hpp"

namespace qat::AST {

class Sentence : public Node {
public:
  Sentence(utils::FileRange _filePlacement) : Node(_filePlacement) {}

  virtual IR::Value *emit(IR::Context *ctx){};

  virtual NodeType nodeType() const {};

  virtual nuo::Json toJson() const {};

  virtual ~Sentence(){};
};

} // namespace qat::AST

#endif