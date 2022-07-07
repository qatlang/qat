#include "./expression_sentence.hpp"

namespace qat {
namespace AST {

ExpressionSentence::ExpressionSentence(Expression *_exp,
                                       utils::FilePlacement _filePlacement)
    : expr(_exp), qat::AST::Sentence(_filePlacement) {}

llvm::Value *ExpressionSentence::emit(IR::Context *ctx) {
  return expr->emit(ctx);
}

void ExpressionSentence::emitCPP(backend::cpp::File &file,
                                 bool isHeader) const {
  if (!isHeader) {
    expr->emitCPP(file, isHeader);
    file += ";\n";
  }
}

backend::JSON ExpressionSentence::toJSON() const {
  return backend::JSON()
      ._("nodeType", "expressionSentence")
      ._("value", expr->toJSON())
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat