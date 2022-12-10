#include "./expression_sentence.hpp"

namespace qat::ast {

ExpressionSentence::ExpressionSentence(Expression* _exp, FileRange _fileRange) : Sentence(_fileRange), expr(_exp) {}

IR::Value* ExpressionSentence::emit(IR::Context* ctx) { return expr->emit(ctx); }

Json ExpressionSentence::toJson() const {
  return Json()._("nodeType", "expressionSentence")._("value", expr->toJson())._("fileRange", fileRange);
}

} // namespace qat::ast