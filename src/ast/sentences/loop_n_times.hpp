#ifndef QAT_AST_SENTENCES_LOOP_N_TIMES_HPP
#define QAT_AST_SENTENCES_LOOP_N_TIMES_HPP

#include "../expression.hpp"
#include "../sentence.hpp"

namespace qat::ast {

/**
 *  LoopNTimes is used to loop through a block of code a provided number
 * of times
 *
 */
class LoopNTimes final : public Sentence {
  Vec<Sentence*>    sentences;
  Expression*       count;
  Maybe<Identifier> tag;

public:
  LoopNTimes(Expression* _count, Vec<Sentence*> _snts, Maybe<Identifier> _tag, FileRange _fileRange)
      : Sentence(_fileRange), sentences(_snts), count(_count), tag(_tag) {}

  useit static inline LoopNTimes* create(Expression* _count, Vec<Sentence*> _snts, Maybe<Identifier> _tag,
                                         FileRange _fileRange) {
    return std::construct_at(OwnNormal(LoopNTimes), _count, _snts, _tag, _fileRange);
  }

  void update_dependencies(IR::EmitPhase phase, Maybe<IR::DependType> dep, IR::EntityState* ent,
                           IR::Context* ctx) final {
    UPDATE_DEPS(count);
    for (auto snt : sentences) {
      UPDATE_DEPS(snt);
    }
  }

  useit bool hasTag() const;
  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::LOOP_N_TIMES; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif