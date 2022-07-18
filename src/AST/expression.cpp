#include "./expression.hpp"

namespace qat::AST {

Expression::Expression(utils::FileRange _filePlacement)
    : expected(ExpressionKind::temporary), Node(_filePlacement) {}

bool Expression::isExpectedKind(ExpressionKind _kind) {
  return (this->expected == _kind);
}

ExpressionKind Expression::getExpectedKind() { return expected; }

void Expression::setExpectedKind(ExpressionKind _kind) {
  this->expected = _kind;
}

} // namespace qat::AST