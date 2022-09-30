#include "./expression.hpp"

namespace qat::ast {

Expression::Expression(utils::FileRange _fileRange)
    : expected(ExpressionKind::temporary), Node(_fileRange) {}

bool Expression::isExpectedKind(ExpressionKind _kind) {
  return (this->expected == _kind);
}

ExpressionKind Expression::getExpectedKind() { return expected; }

void Expression::setExpectedKind(ExpressionKind _kind) {
  this->expected = _kind;
}

ConstantExpression::ConstantExpression(utils::FileRange _range)
    : Expression(std::move(_range)) {}

} // namespace qat::ast