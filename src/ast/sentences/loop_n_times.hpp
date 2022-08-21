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
  String          indexName;

public:
  LoopNTimes(Expression *_count, Vec<Sentence *> _snts, String _indexName,
             utils::FileRange _fileRange);

  useit bool hasName() const;
  useit IR::Value *emit(IR::Context *ctx) final;
  useit NodeType   nodeType() const final { return NodeType::loopTimes; }
  useit nuo::Json toJson() const final;
};

} // namespace qat::ast

#endif