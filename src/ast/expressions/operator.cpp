#include "./operator.hpp"

namespace qat::ast {

Op operator_from_string(const String& str) {
  if (str == "+") {
    return Op::add;
  } else if (str == "-") {
    return Op::subtract;
  } else if (str == "*") {
    return Op::multiply;
  } else if (str == "/") {
    return Op::divide;
  } else if (str == "%") {
    return Op::remainder;
  } else if (str == "|") {
    return Op::bitwiseOr;
  } else if (str == "&") {
    return Op::bitwiseAnd;
  } else if (str == "^") {
    return Op::bitwiseXor;
  } else if (str == "<<") {
    return Op::logicalLeftShift;
  } else if (str == ">>") {
    return Op::logicalRightShift;
  } else if (str == ">>>") {
    return Op::arithmeticRightShift;
  } else if (str == "==") {
    return Op::equalTo;
  } else if (str == "!=") {
    return Op::notEqualTo;
  } else if (str == "<") {
    return Op::lessThan;
  } else if (str == "<=") {
    return Op::lessThanOrEqualTo;
  } else if (str == ">") {
    return Op::greaterThan;
  } else if (str == ">=") {
    return Op::greaterThanEqualTo;
  } else if (str == "&&") {
    return Op::And;
  } else if (str == "||") {
    return Op::Or;
  } else if (str == "[]") {
    return Op::Index;
  } else if (str == "!") {
    return Op::Not;
  } else if (str == "@") {
    return Op::dereference;
  }
} // NOLINT(clang-diagnostic-return-type)

String operator_to_string(Op opr) {
  switch (opr) {
    case Op::add:
      return "+";
    case Op::minus:
    case Op::subtract:
      return "-";
    case Op::multiply:
      return "*";
    case Op::divide:
      return "/";
    case Op::remainder:
      return "%";
    case Op::bitwiseOr:
      return "|";
    case Op::bitwiseAnd:
      return "&";
    case Op::bitwiseXor:
      return "^";
    case Op::logicalLeftShift:
      return "<<";
    case Op::logicalRightShift:
      return ">>";
    case Op::arithmeticRightShift:
      return ">>>";
    case Op::equalTo:
      return "==";
    case Op::notEqualTo:
      return "!=";
    case Op::lessThan:
      return "<";
    case Op::lessThanOrEqualTo:
      return "<=";
    case Op::greaterThan:
      return ">";
    case Op::greaterThanEqualTo:
      return ">=";
    case Op::And:
      return "&&";
    case Op::Or:
      return "||";
    case Op::Index:
      return "[]";
    case Op::Not:
      return "!";
    case Op::copyAssignment:
      return "copy =";
    case Op::moveAssignment:
      return "move =";
    case Op::dereference:
      return "@";
  }
}

} // namespace qat::ast