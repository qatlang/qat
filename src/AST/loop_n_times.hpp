#ifndef QAT_AST_LOOP_N_TIMES_HPP
#define QAT_AST_LOOP_N_TIMES_HPP

#include "../IR/generator.hpp"
#include "../utils/random_number.hpp"
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
 *  LoopNTimes is used to loop through a block of code a provided number
 * of times
 *
 */
class LoopNTimes : public Sentence {
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
   *  Expression representing the number of times it should loop
   *
   */
  Expression count;

public:
  /**
   *  LoopNTimes is used to loop a specified number of times
   *
   * @param _count The expression that gives the number of loop executions
   * @param _block The main block of the loop
   * @param _after The block after the loop, that is not part of the loop
   * @param _filePlacement
   */
  LoopNTimes(Expression _count, Block _block, Block _after,
             utils::FilePlacement _filePlacement);

  /**
   *  Get the index name for the loop index for this sentence
   *
   * @param bb Any BasicBlock inside the parent function
   * @return unsigned The new index id
   */
  unsigned new_loop_index_id(llvm::BasicBlock *bb);

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
  NodeType nodeType() { return NodeType::loopTimes; }
};

} // namespace AST
} // namespace qat

#endif