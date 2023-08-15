add_library(QAT_AST_EXPRESSIONS
	array_literal.cpp
	await.cpp
	binary_expression.cpp
	constructor_call.cpp
	copy.cpp
	default.cpp
	dereference.cpp
	entity.cpp
	function_call.cpp
	heap.cpp
	is.cpp
	loop_index.cpp
	member_function_call.cpp
	index_access.cpp
	member_access.cpp
	move.cpp
	none.cpp
	not.cpp
	operator.cpp
	plain_initialiser.cpp
	radix_literal.cpp
	self.cpp
	self_member.cpp
	generic_entity.cpp
	ternary.cpp
	to_conversion.cpp
	tuple_value.cpp
	mix_type_initialiser.cpp)

target_link_directories(QAT_AST_EXPRESSIONS PUBLIC ../../utils)

target_include_directories(QAT_AST_EXPRESSIONS PUBLIC "${LLVM_ROOT_DIR}/include")

target_link_libraries(QAT_AST_EXPRESSIONS QAT_UTILS)