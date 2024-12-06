#ifndef QAT_AST_SENTENCES_LOOP_TO_HPP
#define QAT_AST_SENTENCES_LOOP_TO_HPP

#include "../expression.hpp"
#include "../sentence.hpp"

namespace qat::ast {

class LoopTo final : public Sentence {
  Vec<Sentence*>    sentences;
  Expression*       count;
  Maybe<Identifier> tag;

public:
  LoopTo(Expression* _count, Vec<Sentence*> _snts, Maybe<Identifier> _tag, FileRange _fileRange)
      : Sentence(_fileRange), sentences(_snts), count(_count), tag(_tag) {}

  useit static LoopTo* create(Expression* _count, Vec<Sentence*> _snts, Maybe<Identifier> _tag, FileRange _fileRange) {
    return std::construct_at(OwnNormal(LoopTo), _count, _snts, _tag, _fileRange);
  }

  void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
    UPDATE_DEPS(count);
    for (auto snt : sentences) {
      UPDATE_DEPS(snt);
    }
  }

  useit bool hasTag() const;
  useit ir::Value* emit(EmitCtx* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::LOOP_N_TIMES; }
  useit Json       to_json() const final;
};

} // namespace qat::ast

#endif
