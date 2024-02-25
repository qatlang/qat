#ifndef QAT_AST_EXPRESSION_SPAWN_HPP
#define QAT_AST_EXPRESSION_SPAWN_HPP

#include "../expression.hpp"
#include "../sentence.hpp"

namespace qat::ast {

class Spawn : public Expression {
  Vec<Sentence*> sentences;

public:
  Spawn(Vec<Sentence*> _sentences, FileRange _fileRange) : Expression(_fileRange), sentences(_sentences) {}

  useit static inline Spawn* create(Vec<Sentence*> sentences, FileRange fileRange) {
    return std::construct_at(OwnNormal(Spawn), sentences, fileRange);
  }

  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::SPAWN; }
};

} // namespace qat::ast

#endif