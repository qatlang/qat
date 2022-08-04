#include "./ternary.hpp"

namespace qat::ast {

TernaryExpression::TernaryExpression(Expression *     _condition,
                                     Expression *     _ifExpression,
                                     Expression *     _elseExpression,
                                     utils::FileRange _fileRange)
    : condition(_condition), if_expr(_ifExpression), else_expr(_elseExpression),
      Expression(std::move(_fileRange)) {}

IR::Value *TernaryExpression::emit(IR::Context *ctx) {
  // TODO - Implement this
}

nuo::Json TernaryExpression::toJson() const {
  return nuo::Json()
      ._("nodeType", "ternaryExpression")
      ._("condition", condition->toJson())
      ._("ifCase", if_expr->toJson())
      ._("elseCase", else_expr->toJson())
      ._("fileRange", fileRange);
}

} // namespace qat::ast