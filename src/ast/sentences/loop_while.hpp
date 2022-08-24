#ifndef QAT_AST_SENTENCES_LOOP_WHILE_HPP
#define QAT_AST_SENTENCES_LOOP_WHILE_HPP

#include "../expression.hpp"
#include "../node_type.hpp"
#include "../sentence.hpp"
#include "./block.hpp"

namespace qat::ast {

/**
 *  LoopWhile is used to loop a block of code while a condition is true
 *
 */
class LoopWhile : public Sentence {
  Expression     *condition;
  Vec<Sentence *> sentences;
  Maybe<String>   tag;

public:
  LoopWhile(Expression *_condition, Vec<Sentence *> _sentences,
            Maybe<String> _tag, utils::FileRange _fileRange);

  useit IR::Value *emit(IR::Context *ctx) final;
  useit nuo::Json toJson() const final;
  useit NodeType  nodeType() const final { return NodeType::loopWhile; }
};

} // namespace qat::ast

#endif