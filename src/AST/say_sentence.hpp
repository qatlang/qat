#ifndef QAT_AST_SAY_SENTENCE_HPP
#define QAT_AST_SAY_SENTENCE_HPP

#include "./expression.hpp"
#include "./sentence.hpp"

namespace qat {
namespace AST {
class SaySentence : public Sentence {
private:
  std::vector<Expression *> expressions;

public:
  SaySentence(std::vector<Expression *> _expressions,
              utils::FilePlacement _filePlacement)
      : expressions(_expressions), Sentence(_filePlacement) {}

  IR::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  NodeType nodeType() const { return NodeType::saySentence; }

  backend::JSON toJSON() const;
};
} // namespace AST
} // namespace qat

#endif