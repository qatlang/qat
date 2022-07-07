#ifndef QAT_AST_LOOP_WHILE_HPP
#define QAT_AST_LOOP_WHILE_HPP

#include "../IR/generator.hpp"
#include "../utils/variability.hpp"
#include "./block.hpp"
#include "./expression.hpp"
#include "./node_type.hpp"
#include "./sentence.hpp"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Value.h"

namespace qat {
namespace AST {

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

  llvm::Value *emit(IR::Generator *generator);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  NodeType nodeType() const { return NodeType::loopWhile; }
};

} // namespace AST
} // namespace qat

#endif