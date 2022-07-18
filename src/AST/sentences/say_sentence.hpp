#ifndef QAT_AST_SENTENCES_SAY_HPP
#define QAT_AST_SENTENCES_SAY_HPP

#include "../expression.hpp"
#include "../sentence.hpp"

namespace qat {
namespace AST {
class Say : public Sentence {
private:
  std::vector<Expression *> expressions;

public:
  Say(std::vector<Expression *> _expressions,
      utils::FilePlacement _filePlacement)
      : expressions(_expressions), Sentence(_filePlacement) {}

  IR::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  NodeType nodeType() const { return NodeType::saySentence; }

  nuo::Json toJson() const;
};
} // namespace AST
} // namespace qat

#endif