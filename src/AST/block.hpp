#ifndef QAT_AST_BLOCK_HPP
#define QAT_AST_BLOCK_HPP

#include "../IR/generator.hpp"
#include "./node_type.hpp"
#include "./sentence.hpp"
#include "llvm/IR/Value.h"
#include <llvm/IR/BasicBlock.h>
#include <sstream>
#include <vector>

namespace qat {
namespace AST {
/**
 * @brief Block represents a group of sentences. This is built so that each
 * block in the language has a unique index within the function
 *
 */
class Block : public Sentence {
private:
  /**
   * @brief All sentences in the block
   *
   */
  std::vector<Sentence> sentences;

  /**
   * @brief BasicBlock corresponding to this Block
   *
   */
  llvm::BasicBlock *bb;

public:
  /**
   * @brief Block represents a group of sentences
   *
   * @param _sentences All sentences in the block
   * @param _filePlacement
   */
  Block(std::vector<Sentence> _sentences, utils::FilePlacement _filePlacement);

  /**
   * @brief Creates a BasicBlock instance associated with the current function.
   * This is used when the llvm::BasicBlock* associated with this Block is
   * required before the generation of the contents of the block happens
   *
   * @return llvm::BasicBlock*
   */
  llvm::BasicBlock *create_bb(IR::Generator *generator);

  /**
   * @brief This is the code generator function that handles the generation of
   * LLVM IR
   *
   * @param generator The IR::Generator instance that handles LLVM IR Generation
   * @return llvm::Value*
   */
  llvm::Value *generate(IR::Generator *generator);

  /**
   * @brief Type of the node represented by this AST member
   *
   * @return NodeType
   */
  NodeType nodeType() { return NodeType::block; }
};
} // namespace AST
} // namespace qat

#endif