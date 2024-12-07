#ifndef QAT_AST_TYPES_TYPE_KIND_HPP
#define QAT_AST_TYPES_TYPE_KIND_HPP

namespace qat::ast {

/**
 *  The nature of type of the Type instance.
 *
 * I almost named this TypeType
 *
 */
enum class AstTypeKind {
	VOID,
	ARRAY,
	NAMED,
	GENERIC_NAMED,
	FLOAT,
	INTEGER,
	UNSIGNED_INTEGER,
	STRING_SLICE,
	C_TYPE,
	TUPLE,
	POINTER,
	REFERENCE,
	FUNCTION,
	FUTURE,
	GENERIC_ABSTRACT,
	MAYBE,
	LINKED_GENERIC,
	RESULT,
	SELF_TYPE,
	VECTOR,
	GENERIC_INTEGER,
};

} // namespace qat::ast

#endif