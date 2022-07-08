#ifndef QAT_AST_BLOCK_HPP
#define QAT_AST_BLOCK_HPP

#include "../IR/context.hpp"
#include "../utils/new_block_index.hpp"
#include "./node_type.hpp"
#include "./sentence.hpp"
#include "llvm/IR/Value.h"
#include <llvm/IR/BasicBlock.h>
#include <sstream>
#include <vector>

namespace qat {
namespace AST {

/**
 *  Block represents a group of sentences. This is built so that each
 * block in the language has a unique index within the function
 *
 */
class Block : public Sentence {
private:
  /**
   *  All sentences in the block
   *
   */
  std::vector<Sentence *> sentences;

  /**
   *  BasicBlock corresponding to this Block
   *
   */
  llvm::BasicBlock *bb;

  /**
   *  The ending BasicBlock in this block. This is different if there are
   * nested sentences, if-else, multithread, loops and similar blocky sentences
   *
   */
  llvm::BasicBlock *end_bb;

  /**
   *  Set the block that ends the scope of the current block
   *
   * @param ctx LLVMContext
   * @param end_block Name of the ending block
   */
  void set_alloca_scope_end(llvm::LLVMContext &ctx,
                            std::string end_block) const;

public:
  /**
   *  Block represents a group of sentences
   *
   * @param _sentences All sentences in the block
   * @param _filePlacement
   */
  Block(std::vector<Sentence *> _sentences,
        utils::FilePlacement _filePlacement);

  /**
   *  Creates a BasicBlock instance associated with the current function.
   * This is used when the llvm::BasicBlock* associated with this Block is
   * required before the generation of the contents of the block happens
   *
   * @param ctx IR Generator
   * @param function Function to insert the BasicBlock into
   *
   * @return llvm::BasicBlock*
   */
  llvm::BasicBlock *create_bb(IR::Context *ctx,
                              llvm::Function *function = nullptr);

  /**
   *  Get the BasicBlock that ends the scope of this block
   *
   * @return llvm::BasicBlock*
   */
  llvm::BasicBlock *get_end_block() const;

  IR::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  backend::JSON toJSON() const;

  NodeType nodeType() const { return NodeType::block; }
};

} // namespace AST
} // namespace qat

#endif