add_library(QAT_AST_EXPRESSIONS
	address_of.cpp
	array_literal.cpp
	await.cpp
	binary_expression.cpp
	cast.cpp
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
	negative.cpp
	not.cpp
	ok.cpp
	operator.cpp
	plain_initialiser.cpp
	self_instance.cpp
	generic_entity.cpp
	to_conversion.cpp
	tuple_value.cpp
	mix_choice_initialiser.cpp)

target_link_directories(QAT_AST_EXPRESSIONS PUBLIC ../../utils)

target_include_directories(QAT_AST_EXPRESSIONS PUBLIC "${LLVM_ROOT_DIR}/include")

target_link_libraries(QAT_AST_EXPRESSIONS QAT_UTILS)