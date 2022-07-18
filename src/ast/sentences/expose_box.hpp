#ifndef QAT_AST_SENTENCES_EXPOSE_SPACE_HPP
#define QAT_AST_SENTENCES_EXPOSE_SPACE_HPP

#include "../sentence.hpp"

namespace qat::ast {

class ExposeBoxes : public Sentence {
private:
  std::vector<std::string> boxes;
  std::vector<Sentence *> sentences;

public:
  ExposeBoxes(std::vector<std::string> _boxes,
              std::vector<Sentence *> _sentences, utils::FileRange _fileRange)
      : Sentence(_fileRange), boxes(_boxes), sentences(_sentences) {}

  IR::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  NodeType nodeType() const { return NodeType::exposeBoxes; }

  nuo::Json toJson() const;
};

} // namespace qat::ast

#endif