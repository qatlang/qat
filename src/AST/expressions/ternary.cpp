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

backend::JSON TernaryExpression::toJSON() const {
  return backend::JSON()
      ._("nodeType", "ternaryExpression")
      ._("condition", condition->toJSON())
      ._("ifCase", if_expr->toJSON())
      ._("elseCase", else_expr->toJSON())
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat