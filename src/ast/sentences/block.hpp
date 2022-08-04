#ifndef QAT_AST_SENTENCES_BLOCK_HPP
#define QAT_AST_SENTENCES_BLOCK_HPP

#include "../sentence.hpp"
#include <vector>

namespace qat::ast {

/**
 * Block represents a group of sentences. This is built so that each
 * block in the language has a unique index within the function
 *
 */
class Block : public Sentence {
private:
  /**
   *  All sentences in the block
   *
   */
  Vec<Sentence *> sentences;

public:
  /**
   *  Block represents a group of sentences
   *
   * @param _sentences All sentences in the block
   * @param _fileRange
   */
  Block(Vec<Sentence *> _sentences, utils::FileRange _fileRange);

  IR::Value *emit(IR::Context *ctx);

  nuo::Json toJson() const;

  NodeType nodeType() const { return NodeType::block; }
};

} // namespace qat::ast

#endif