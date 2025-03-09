#include "./operator.hpp"

namespace qat::ast {

usize get_precedence_of(OperatorKind Operator) {
	switch (Operator) {
		case OperatorKind::MULTIPLY:
		case OperatorKind::DIVIDE:
		case OperatorKind::REMAINDER: {
			return 10;
		}
		case OperatorKind::ADDITION:
		case OperatorKind::SUBTRACT: {
			return 20;
		}
		case OperatorKind::LOGICAL_LEFT_SHIFT:
		case OperatorKind::LOGICAL_RIGHT_SHIFT:
		case OperatorKind::ARITHMETIC_RIGHT_SHIFT: {
			return 30;
		}
		case OperatorKind::GREATER_THAN:
		case OperatorKind::GREATER_THAN_OR_EQUAL_TO:
		case OperatorKind::LESS_THAN:
		case OperatorKind::LESS_THAN_OR_EQUAL_TO: {
			return 40;
		}
		case OperatorKind::EQUAL_TO:
		case OperatorKind::NOT_EQUAL_TO:
			return 50;
		case OperatorKind::BITWISE_AND:
			return 50;
		case OperatorKind::BITWISE_XOR:
			return 60;
		case OperatorKind::BITWISE_OR:
			return 70;
		case OperatorKind::AND:
			return 80;
		case OperatorKind::OR:
			return 90;
		default:
			return 500;
	}
}

OperatorKind operator_from_string(const String& str) {
	if (str == "+") {
		return OperatorKind::ADDITION;
	} else if (str == "-") {
		return OperatorKind::SUBTRACT;
	} else if (str == "*") {
		return OperatorKind::MULTIPLY;
	} else if (str == "/") {
		return OperatorKind::DIVIDE;
	} else if (str == "%") {
		return OperatorKind::REMAINDER;
	} else if (str == "|") {
		return OperatorKind::BITWISE_OR;
	} else if (str == "&") {
		return OperatorKind::BITWISE_AND;
	} else if (str == "^") {
		return OperatorKind::BITWISE_XOR;
	} else if (str == "<<") {
		return OperatorKind::LOGICAL_LEFT_SHIFT;
	} else if (str == ">>") {
		return OperatorKind::LOGICAL_RIGHT_SHIFT;
	} else if (str == ">>>") {
		return OperatorKind::ARITHMETIC_RIGHT_SHIFT;
	} else if (str == "==") {
		return OperatorKind::EQUAL_TO;
	} else if (str == "!=") {
		return OperatorKind::NOT_EQUAL_TO;
	} else if (str == "<") {
		return OperatorKind::LESS_THAN;
	} else if (str == "<=") {
		return OperatorKind::LESS_THAN_OR_EQUAL_TO;
	} else if (str == ">") {
		return OperatorKind::GREATER_THAN;
	} else if (str == ">=") {
		return OperatorKind::GREATER_THAN_OR_EQUAL_TO;
	} else if (str == "&&") {
		return OperatorKind::AND;
	} else if (str == "||") {
		return OperatorKind::OR;
	} else if (str == "[]") {
		return OperatorKind::INDEX;
	} else if (str == "!") {
		return OperatorKind::NOT;
	} else if (str == "@") {
		return OperatorKind::DEREFERENCE;
	} else if (str == "~") {
		return OperatorKind::BITWISE_NOT;
	}
} // NOLINT(clang-diagnostic-return-type)

String operator_to_string(OperatorKind opr) {
	switch (opr) {
		case OperatorKind::ADDITION:
			return "+";
		case OperatorKind::MINUS:
		case OperatorKind::SUBTRACT:
			return "-";
		case OperatorKind::MULTIPLY:
			return "*";
		case OperatorKind::DIVIDE:
			return "/";
		case OperatorKind::REMAINDER:
			return "%";
		case OperatorKind::BITWISE_OR:
			return "|";
		case OperatorKind::BITWISE_AND:
			return "&";
		case OperatorKind::BITWISE_XOR:
			return "^";
		case OperatorKind::LOGICAL_LEFT_SHIFT:
			return "<<";
		case OperatorKind::LOGICAL_RIGHT_SHIFT:
			return ">>";
		case OperatorKind::ARITHMETIC_RIGHT_SHIFT:
			return ">>>";
		case OperatorKind::EQUAL_TO:
			return "==";
		case OperatorKind::NOT_EQUAL_TO:
			return "!=";
		case OperatorKind::LESS_THAN:
			return "<";
		case OperatorKind::LESS_THAN_OR_EQUAL_TO:
			return "<=";
		case OperatorKind::GREATER_THAN:
			return ">";
		case OperatorKind::GREATER_THAN_OR_EQUAL_TO:
			return ">=";
		case OperatorKind::AND:
			return "&&";
		case OperatorKind::OR:
			return "||";
		case OperatorKind::INDEX:
			return "[]";
		case OperatorKind::NOT:
			return "!";
		case OperatorKind::COPY_ASSIGNMENT:
			return "copy =";
		case OperatorKind::MOVE_ASSIGNMENT:
			return "move =";
		case OperatorKind::DEREFERENCE:
			return "@";
		case OperatorKind::BITWISE_NOT:
			return "~";
	}
}

} // namespace qat::ast
