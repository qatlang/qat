#ifndef QAT_AST_SENTENCES_LOOP_WHILE_HPP
#define QAT_AST_SENTENCES_LOOP_WHILE_HPP

#include "../expression.hpp"
#include "../node_type.hpp"
#include "../sentence.hpp"
#include "./block.hpp"

namespace qat::AST {

/**
 *  LoopWhile is used to loop a block of code while a condition is true
 *
 */
class LoopWhile : public Sentence {
  /**
   *  The block that will be looped over
   *
   */
  Block *block;

  /**
   *  The block that happens after the loop block
   *
   */
  Block *after;

  /**
   *  Expression representing the condition for the loop
   *
   */
  Expression *condition;

public:
  /**
   *  LoopWhile is used to loop through a block of code while a condition
   * is true
   *
   * @param _condition Condition that controls the number of times the loop
   * executes
   * @param _block The main block of the loop
   * @param _after The block after the loop, which is not part of it
   * @param _filePlacement
   */
  LoopWhile(Expression *_condition, Block *_block, Block *_after,
            utils::FilePlacement _filePlacement);

  IR::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  nuo::Json toJson() const;

  NodeType nodeType() const { return NodeType::loopWhile; }
};

} // namespace qat::AST

#endif