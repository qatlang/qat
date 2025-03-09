#ifndef QAT_IR_OPERATOR_HPP
#define QAT_IR_OPERATOR_HPP

#include "../../utils/helpers.hpp"
#include "../../utils/macros.hpp"

namespace qat::ast {

enum class OperatorKind {
	ADDITION,
	SUBTRACT,
	MULTIPLY,
	DIVIDE,
	REMAINDER,
	BITWISE_OR,
	BITWISE_AND,
	BITWISE_XOR,
	BITWISE_NOT,
	LOGICAL_LEFT_SHIFT,
	LOGICAL_RIGHT_SHIFT,
	ARITHMETIC_RIGHT_SHIFT,
	EQUAL_TO,
	NOT_EQUAL_TO,
	LESS_THAN,
	LESS_THAN_OR_EQUAL_TO,
	GREATER_THAN,
	GREATER_THAN_OR_EQUAL_TO,
	AND,
	OR,
	INDEX,
	MINUS,
	NOT,
	COPY_ASSIGNMENT,
	MOVE_ASSIGNMENT,
	DEREFERENCE,
};

useit usize get_precedence_of(OperatorKind Operator);

useit inline bool is_unary_operator(OperatorKind opr) {
	switch (opr) {
		case OperatorKind::MINUS:
		case OperatorKind::BITWISE_NOT:
		case OperatorKind::NOT:
		case OperatorKind::DEREFERENCE:
			return true;
		default:
			return false;
	}
}

useit inline bool expect_same_operand_types(OperatorKind opr) {
	switch (opr) {
		case OperatorKind::ADDITION:
		case OperatorKind::SUBTRACT:
		case OperatorKind::MULTIPLY:
		case OperatorKind::DIVIDE:
		case OperatorKind::REMAINDER:
		case OperatorKind::BITWISE_OR:
		case OperatorKind::BITWISE_AND:
		case OperatorKind::BITWISE_XOR:
		case OperatorKind::EQUAL_TO:
		case OperatorKind::NOT_EQUAL_TO:
		case OperatorKind::LESS_THAN:
		case OperatorKind::LESS_THAN_OR_EQUAL_TO:
		case OperatorKind::GREATER_THAN:
		case OperatorKind::GREATER_THAN_OR_EQUAL_TO:
			return true;
		case OperatorKind::AND:
		case OperatorKind::OR:
		case OperatorKind::INDEX:
		case OperatorKind::MINUS:
		case OperatorKind::NOT:
		case OperatorKind::COPY_ASSIGNMENT:
		case OperatorKind::MOVE_ASSIGNMENT:
		case OperatorKind::DEREFERENCE:
		case OperatorKind::LOGICAL_LEFT_SHIFT:
		case OperatorKind::LOGICAL_RIGHT_SHIFT:
		case OperatorKind::ARITHMETIC_RIGHT_SHIFT:
			return false;
	}
}

useit String operator_to_string(OperatorKind opr);

useit OperatorKind operator_from_string(const String& str);

} // namespace qat::ast

#endif
