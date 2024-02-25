#ifndef QAT_AST_SENTENCES_LOOP_WHILE_HPP
#define QAT_AST_SENTENCES_LOOP_WHILE_HPP

#include "../expression.hpp"
#include "../node_type.hpp"
#include "../sentence.hpp"

namespace qat::ast {

/**
 *  LoopWhile is used to loop a block of code while a condition is true
 *
 */
class LoopWhile final : public Sentence {
  Expression*       condition;
  Vec<Sentence*>    sentences;
  Maybe<Identifier> tag;
  bool              isDoAndLoop = false;

public:
  LoopWhile(bool _isDoAndLoop, Expression* _condition, Vec<Sentence*> _sentences, Maybe<Identifier> _tag,
            FileRange _fileRange)
      : Sentence(_fileRange), condition(_condition), sentences(_sentences), tag(_tag), isDoAndLoop(_isDoAndLoop) {}

  useit static inline LoopWhile* create(bool _isDoAndLoop, Expression* _condition, Vec<Sentence*> _sentences,
                                        Maybe<Identifier> _tag, FileRange _fileRange) {
    return std::construct_at(OwnNormal(LoopWhile), _isDoAndLoop, _condition, _sentences, _tag, _fileRange);
  }

  void update_dependencies(IR::EmitPhase phase, Maybe<IR::DependType> dep, IR::EntityState* ent,
                           IR::Context* ctx) final {
    UPDATE_DEPS(condition);
    for (auto snt : sentences) {
      UPDATE_DEPS(snt);
    }
  }

  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::LOOP_WHILE; }
};

} // namespace qat::ast

#endif