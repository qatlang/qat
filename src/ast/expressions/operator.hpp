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
  Or,
  Index,
  minus,
  Not,
  copyAssignment,
  moveAssignment,
  dereference,
};

useit inline bool isUnaryOp(Op opr) {
  switch (opr) {
    case Op::minus:
    case Op::Not:
    case Op::dereference:
      return true;
    default:
      return false;
  }
}

useit inline bool expectSameOperandType(Op opr) {
  switch (opr) {
    case Op::add:
    case Op::subtract:
    case Op::multiply:
    case Op::divide:
    case Op::remainder:
    case Op::bitwiseOr:
    case Op::bitwiseAnd:
    case Op::bitwiseXor:
    case Op::equalTo:
    case Op::notEqualTo:
    case Op::lessThan:
    case Op::lessThanOrEqualTo:
    case Op::greaterThan:
    case Op::greaterThanEqualTo:
      return true;
    case Op::And:
    case Op::Or:
    case Op::Index:
    case Op::minus:
    case Op::Not:
    case Op::copyAssignment:
    case Op::moveAssignment:
    case Op::dereference:
    case Op::logicalLeftShift:
    case Op::logicalRightShift:
    case Op::arithmeticRightShift:
      return false;
  }
}

useit String OpToString(Op opr);

useit Op OpFromString(const String& str);

} // namespace qat::ast

#endif
