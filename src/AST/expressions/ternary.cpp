#include "./ternary.hpp"

namespace qat {
namespace AST {

TernaryExpression::TernaryExpression(Expression *_condition,
                                     Expression *_ifExpression,
                                     Expression *_elseExpression,
                                     utils::FilePlacement _filePlacement)
    : condition(_condition), if_expr(_ifExpression), else_expr(_elseExpression),
      Expression(_filePlacement) {}

IR::Value *TernaryExpression::emit(qat::IR::Context *ctx) {
  // TODO - Implement this
}

void TernaryExpression::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (!isHeader) {
    file += "((";
    condition->emitCPP(file, isHeader);
    file += ") ? (";
    if_expr->emitCPP(file, isHeader);
    file += ") : (";
    else_expr->emitCPP(file, isHeader);
    file += ")) ";
  }
}

nuo::Json TernaryExpression::toJson() const {
  return nuo::Json()
      ._("nodeType", "ternaryExpression")
      ._("condition", condition->toJson())
      ._("ifCase", if_expr->toJson())
      ._("elseCase", else_expr->toJson())
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat