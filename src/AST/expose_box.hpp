#ifndef QAT_AST_EXPOSE_SPACE_HPP
#define QAT_AST_EXPOSE_SPACE_HPP

#include "./box.hpp"
#include "./node_type.hpp"
#include "./sentence.hpp"

namespace qat {
namespace AST {
class ExposeBoxes : public Sentence {
private:
  std::vector<Box> boxes;
  std::vector<Sentence> sentences;

public:
  ExposeBoxes(std::vector<Box> _boxes, std::vector<Sentence> _sentences,
              utils::FilePlacement _filePlacement)
      : boxes(_boxes), sentences(_sentences), Sentence(_filePlacement) {}

  llvm::Value *generate(IR::Generator *generator);

  NodeType nodeType() { return qat::AST::NodeType::exposeBoxes; }

  backend::JSON toJSON() const;
};
} // namespace AST
} // namespace qat

#endif