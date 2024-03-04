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

  void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
    for (auto snt : sentences) {
      UPDATE_DEPS(snt);
    }
  }

  useit ir::Value* emit(EmitCtx* ctx) final;
  useit Json       to_json() const final;
  useit NodeType   nodeType() const final { return NodeType::LOOP_NORMAL; }
};

} // namespace qat::ast

#endif