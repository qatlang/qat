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

  /**
   * @brief The ending BasicBlock in this block. This is different if there are
   * nested sentences, if-else, multithread, loops and similar blocky sentences
   *
   */
  llvm::BasicBlock *end_bb;

  /**
   * @brief Set the block that ends the scope of the current block
   *
   * @param ctx LLVMContext
   * @param end_block Name of the ending block
   */
  void set_alloca_scope_end(llvm::LLVMContext &ctx,
                            std::string end_block) const;

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
   * @param generator IR Generator
   * @param function Function to insert the BasicBlock into
   *
   * @return llvm::BasicBlock*
   */
  llvm::BasicBlock *create_bb(IR::Generator *generator,
                              llvm::Function *function = nullptr);

  /**
   * @brief Get the BasicBlock that ends the scope of this block
   *
   * @return llvm::BasicBlock*
   */
  llvm::BasicBlock *get_end_block() const;

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