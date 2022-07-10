#ifndef QAT_AST_SENTENCES_BLOCK_HPP
#define QAT_AST_SENTENCES_BLOCK_HPP

#include "../sentence.hpp"
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

public:
  /**
   *  Block represents a group of sentences
   *
   * @param _sentences All sentences in the block
   * @param _filePlacement
   */
  Block(std::vector<Sentence *> _sentences,
        utils::FilePlacement _filePlacement);

  IR::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  backend::JSON toJSON() const;

  NodeType nodeType() const { return NodeType::block; }
};

} // namespace AST
} // namespace qat

#endif