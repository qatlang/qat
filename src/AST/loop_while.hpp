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
  Block block;

  /**
   *  The block that happens after the loop block
   *
   */
  Block after;

  /**
   *  Expression representing the condition for the loop
   *
   */
  Expression condition;

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
  LoopWhile(Expression _condition, Block _block, Block _after,
            utils::FilePlacement _filePlacement);

  /**
   *  This is the code generator function that handles the generation of
   * LLVM IR
   *
   * @param generator The IR::Generator instance that handles LLVM IR Generation
   * @return llvm::Value*
   */
  llvm::Value *generate(IR::Generator *generator);

  /**
   *  Type of the node represented by this AST member
   *
   * @return NodeType
   */
  NodeType nodeType() { return NodeType::loopWhile; }
};

} // namespace AST
} // namespace qat

#endif