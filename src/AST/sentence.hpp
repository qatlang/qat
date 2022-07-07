#ifndef QAT_AST_SENTENCE_HPP
#define QAT_AST_SENTENCE_HPP

#include "node.hpp"

namespace qat {
namespace AST {
class Sentence : public Node {
public:
  Sentence(utils::FilePlacement _filePlacement) : Node(_filePlacement) {}

  virtual llvm::Value *emit(IR::Context *ctx){};

  virtual NodeType nodeType(){};

  virtual backend::JSON toJSON() const {};

  virtual ~Sentence(){};
};
} // namespace AST
} // namespace qat

#endif