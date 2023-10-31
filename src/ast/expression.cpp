#include "./expression.hpp"

namespace qat::ast {

Expression::Expression(FileRange _fileRange) : Node(std::move(_fileRange)), expected(ExpressionKind::temporary) {}

bool Expression::isExpectedKind(ExpressionKind _kind) { return (this->expected == _kind); }

ExpressionKind Expression::getExpectedKind() { return expected; }

void Expression::setExpectedKind(ExpressionKind _kind) { this->expected = _kind; }

PrerunExpression::PrerunExpression(FileRange _range) : Expression(std::move(_range)) {}

String PrerunExpression::toString() const { return ""; }

} // namespace qat::ast