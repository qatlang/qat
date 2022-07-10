#ifndef QAT_AST_SENTENCES_EXPOSE_SPACE_HPP
#define QAT_AST_SENTENCES_EXPOSE_SPACE_HPP

#include "../sentence.hpp"

namespace qat {
namespace AST {
class ExposeBoxes : public Sentence {
private:
  std::vector<std::string> boxes;
  std::vector<Sentence *> sentences;

public:
  ExposeBoxes(std::vector<std::string> _boxes,
              std::vector<Sentence *> _sentences,
              utils::FilePlacement _filePlacement)
      : boxes(_boxes), sentences(_sentences), Sentence(_filePlacement) {}

  IR::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  NodeType nodeType() const { return qat::AST::NodeType::exposeBoxes; }

  backend::JSON toJSON() const;
};
} // namespace AST
} // namespace qat

#endif