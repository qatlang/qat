#ifndef QAT_IR_OPERATOR_HPP
#define QAT_IR_OPERATOR_HPP

#include "../../utils/helpers.hpp"
#include "../../utils/macros.hpp"

namespace qat::ast {

enum class Op {
  add,
  subtract,
  multiply,
  divide,
  remainder,
  bitwiseOr,
  bitwiseAnd,
  bitwiseXor,
  logicalLeftShift,
  logicalRightShift,
  arithmeticRightShift,
  equalTo,
  notEqualTo,
  lessThan,
  lessThanOrEqualTo,
  greaterThan,
  greaterThanEqualTo,
  And,
  Or
};

useit String OpToString(Op opr);

useit Op OpFromString(const String &str);

} // namespace qat::ast

#endif
