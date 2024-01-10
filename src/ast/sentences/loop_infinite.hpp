#ifndef QAT_AST_SENTENCES_LOOP_INFINITE_HPP
#define QAT_AST_SENTENCES_LOOP_INFINITE_HPP

#include "../expression.hpp"
#include "../node_type.hpp"
#include "../sentence.hpp"
#include "./block.hpp"

namespace qat::ast {

class LoopInfinite : public Sentence {
  Vec<Sentence*>    sentences;
  Maybe<Identifier> tag;

public:
  LoopInfinite(Vec<Sentence*> _sentences, Maybe<Identifier> _tag, FileRange _fileRange);

  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::loopNormal; }
};

} // namespace qat::ast

#endif