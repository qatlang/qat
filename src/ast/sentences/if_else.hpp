#ifndef QAT_AST_SENTENCES_IF_ELSE_HPP
#define QAT_AST_SENTENCES_IF_ELSE_HPP

#include "../expression.hpp"
#include "../sentence.hpp"
#include "./block.hpp"
#include <optional>

namespace qat::ast {

// `IfElse` is used to represent two kinds of conditional sentences : If
// and If-Else. The Else block is optional and if omitted, IfElse becomes a
// plain if sentence
class IfElse : public Sentence {
private:
  Vec<Pair<Expression*, Vec<Sentence*>>> chain;
  Maybe<Vec<Sentence*>>                  elseCase;
  Vec<Maybe<bool>>                       knownVals;

public:
  IfElse(Vec<Pair<Expression*, Vec<Sentence*>>> _chain, Maybe<Vec<Sentence*>> _else, FileRange _fileRange);

  useit Pair<bool, usize> trueKnownValue(usize ind) const;
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
  useit NodeType   nodeType() const final { return NodeType::ifElse; }
};

} // namespace qat::ast

#endif