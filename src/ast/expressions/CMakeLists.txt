add_library(QAT_AST_EXPRESSIONS STATIC
	address_of.cc
	array_literal.cc
	assembly.cc
	await.cc
	binary_expression.cc
	cast.cc
	constructor_call.cc
	copy.cc
	default.cc
	dereference.cc
	entity.cc
	error.cc
	function_call.cc
	get_intrinsic.cc
	heap.cc
	is.cc
	member_function_call.cc
	index_access.cc
	member_access.cc
	move.cc
	negative.cc
	not.cc
	ok.cc
	operator.cc
	plain_initialiser.cc
	self_instance.cc
	generic_entity.cc
	to_conversion.cc
	tuple_value.cc
	mix_choice_initialiser.cc)

target_link_directories(QAT_AST_EXPRESSIONS PUBLIC ../../utils)

target_include_directories(QAT_AST_EXPRESSIONS PUBLIC "${LLVM_ROOT_DIR}/include")

target_link_libraries(QAT_AST_EXPRESSIONS QAT_UTILS)
