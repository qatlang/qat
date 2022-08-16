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
  Vec<Pair<Expression *, Vec<Sentence *>>> chain;
  Maybe<Vec<Sentence *>>                   elseCase;

public:
  IfElse(Vec<Pair<Expression *, Vec<Sentence *>>> _chain,
         Maybe<Vec<Sentence *>> _else, utils::FileRange _fileRange);

  useit IR::Value *emit(IR::Context *ctx) final;
  useit nuo::Json toJson() const final;
  useit NodeType  nodeType() const final { return NodeType::ifElse; }
};

} // namespace qat::ast

#endif