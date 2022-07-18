#ifndef QAT_AST_SENTENCES_MULTITHREAD_GIVE_HPP
#define QAT_AST_SENTENCES_MULTITHREAD_GIVE_HPP

#include "../expression.hpp"
#include "../sentence.hpp"

namespace qat::ast {

/**
 *  MultithreadGive represents a give sentence (return statement) inside a
 * multithread block.
 *
 */
class MultithreadGive : public Sentence {
private:
  /**
   *  Expression to be returned from the multithread block.
   *
   */
  Expression *expression;

public:
  /**
   *  Add a multithread give sentence to the parent multithread block
   *
   * @param _expression Expression to be given from the parent multithread
   block
   * @param _fileRange
   */
  MultithreadGive(Expression *_expression, utils::FileRange _fileRange);

  IR::Value *emit(IR::Context *ctx);

  nuo::Json toJson() const;

  NodeType nodeType() const { return NodeType::multithreadGive; }
};

} // namespace qat::ast

#endif