#ifndef QAT_AST_SENTENCE_HPP
#define QAT_AST_SENTENCE_HPP

#include "./node.hpp"

namespace qat::ast {

class Sentence : public Node {
public:
  Sentence(utils::FileRange _fileRange) : Node(std::move(_fileRange)) {}

  useit IR::Value *emit(IR::Context *ctx) override{};

  useit NodeType nodeType() const override{};

  useit nuo::Json toJson() const override{};

  void destroy() override{};

  ~Sentence() override = default;
};

} // namespace qat::ast

#endif