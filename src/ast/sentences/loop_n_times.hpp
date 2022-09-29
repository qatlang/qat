#ifndef QAT_AST_SENTENCES_LOOP_N_TIMES_HPP
#define QAT_AST_SENTENCES_LOOP_N_TIMES_HPP

#include "../expression.hpp"
#include "../sentence.hpp"
#include "./block.hpp"

namespace qat::ast {

/**
 *  LoopNTimes is used to loop through a block of code a provided number
 * of times
 *
 */
class LoopNTimes : public Sentence {
  Vec<Sentence *> sentences;
  Expression     *count;
  Maybe<String>   tag;
  bool            isAlias;

public:
  LoopNTimes(Expression *_count, Vec<Sentence *> _snts, Maybe<String> _tag,
             bool _isAlias, utils::FileRange _fileRange);

  useit bool hasTag() const;
  useit IR::Value *emit(IR::Context *ctx) final;
  useit NodeType   nodeType() const final { return NodeType::loopTimes; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif