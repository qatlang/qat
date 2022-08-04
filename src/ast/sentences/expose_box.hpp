#ifndef QAT_AST_SENTENCES_EXPOSE_SPACE_HPP
#define QAT_AST_SENTENCES_EXPOSE_SPACE_HPP

#include "../sentence.hpp"

namespace qat::ast {

class ExposeBoxes : public Sentence {
private:
  Vec<String>     boxes;
  Vec<Sentence *> sentences;

public:
  ExposeBoxes(Vec<String> _boxes, Vec<Sentence *> _sentences,
              utils::FileRange _fileRange)
      : Sentence(_fileRange), boxes(_boxes), sentences(_sentences) {}

  IR::Value *emit(IR::Context *ctx);

  NodeType nodeType() const { return NodeType::exposeBoxes; }

  nuo::Json toJson() const;
};

} // namespace qat::ast

#endif