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
  // All sentences in the block
  Vec<Sentence *> sentences;
  IR::Block      *irBlock;

public:
  Block(Vec<Sentence *> _sentences, utils::FileRange _fileRange);

  useit IR::Value *emit(IR::Context *ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::block; }
};

} // namespace qat::ast

#endif