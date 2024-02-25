#ifndef QAT_AST_SENTENCES_LOOP_INFINITE_HPP
#define QAT_AST_SENTENCES_LOOP_INFINITE_HPP

#include "../node_type.hpp"
#include "../sentence.hpp"

namespace qat::ast {

class LoopInfinite final : public Sentence {
  Vec<Sentence*>    sentences;
  Maybe<Identifier> tag;

public:
  LoopInfinite(Vec<Sentence*> _sentences, Maybe<Identifier> _tag, FileRange _fileRange)
      : Sentence(_fileRange), sentences(_sentences), tag(_tag) {}

  useit static inline LoopInfinite* create(Vec<Sentence*> _sentences, Maybe<Identifier> _tag, FileRange _fileRange) {
    return std::construct_at(OwnNormal(LoopInfinite), _sentences, _tag, _fileRange);
  }

  void update_dependencies(IR::EmitPhase phase, Maybe<IR::DependType> dep, IR::EntityState* ent,
                           IR::Context* ctx) final {
    for (auto snt : sentences) {
      UPDATE_DEPS(snt);
    }
  }

  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::LOOP_NORMAL; }
};

} // namespace qat::ast

#endif