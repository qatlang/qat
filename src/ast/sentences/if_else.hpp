#ifndef QAT_AST_SENTENCES_IF_ELSE_HPP
#define QAT_AST_SENTENCES_IF_ELSE_HPP

#include "../expression.hpp"
#include "../sentence.hpp"
#include <optional>

namespace qat::ast {

// `IfElse` is used to represent two kinds of conditional sentences : If
// and If-Else. The Else block is optional and if omitted, IfElse becomes a
// plain if sentence
class IfElse final : public Sentence {
  Vec<std::tuple<Expression*, Vec<Sentence*>, FileRange>> chain;
  Maybe<Pair<Vec<Sentence*>, FileRange>>                  elseCase;
  Vec<Maybe<bool>>                                        knownVals;

public:
  IfElse(Vec<std::tuple<Expression*, Vec<Sentence*>, FileRange>> _chain, Maybe<Pair<Vec<Sentence*>, FileRange>> _else,
         FileRange _fileRange)
      : Sentence(_fileRange), chain(_chain), elseCase(_else) {}

  useit static inline IfElse* create(Vec<std::tuple<Expression*, Vec<Sentence*>, FileRange>> _chain,
                                     Maybe<Pair<Vec<Sentence*>, FileRange>> _else, FileRange _fileRange) {
    return std::construct_at(OwnNormal(IfElse), _chain, _else, _fileRange);
  }

  void update_dependencies(IR::EmitPhase phase, Maybe<IR::DependType> dep, IR::EntityState* ent,
                           IR::Context* ctx) final {
    for (auto& ch : chain) {
      UPDATE_DEPS(std::get<0>(ch));
      for (auto snt : std::get<1>(ch)) {
        UPDATE_DEPS(snt);
      }
    }
    if (elseCase.has_value()) {
      for (auto snt : elseCase.value().first) {
        UPDATE_DEPS(snt);
      }
    }
  }

  useit Pair<bool, usize> trueKnownValueBefore(usize ind) const;
  useit bool              getKnownValue(usize ind) const;
  useit bool              hasValueAt(usize ind) const;
  useit bool              isFalseTill(usize ind) const;
  useit bool              hasAnyKnownValue() const {
    for (const auto& val : knownVals) {
      if (val.has_value()) {
        return true;
      }
    }
    return false;
  };
  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::IF_ELSE_IF; }
};

} // namespace qat::ast

#endif