#include "./expression_sentence.hpp"

namespace qat::ast {

ExpressionSentence::ExpressionSentence(Expression *_exp,
                                       utils::FileRange _fileRange)
    : Sentence(_fileRange), expr(_exp) {}

IR::Value *ExpressionSentence::emit(IR::Context *ctx) {
  return expr->emit(ctx);
}

void ExpressionSentence::emitCPP(backend::cpp::File &file,
                                 bool isHeader) const {
  if (!isHeader) {
    expr->emitCPP(file, isHeader);
    file += ";\n";
  }
}

nuo::Json ExpressionSentence::toJson() const {
  return nuo::Json()
      ._("nodeType", "expressionSentence")
      ._("value", expr->toJson())
      ._("fileRange", fileRange);
}

} // namespace qat::ast