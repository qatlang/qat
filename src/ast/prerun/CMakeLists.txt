add_library(QAT_AST_PRERUN
	array_literal.cc
	binary_op.cc
	bitwise_not.cc
	boolean_literal.cc
	custom_float_literal.cc
	custom_integer_literal.cc
	default.cc
	entity.cc
	flag_initialiser.cc
	float_literal.cc
	function_call.cc
	integer_literal.cc
	member_access.cc
	method_call.cc
	mix_choice_init.cc
	negative.cc
	none.cc
	null_mark.cc
	plain_initialiser.cc
	prerun_global.cc
	string_literal.cc
	sub_entity.cc
	to_conversion.cc
	tuple_value.cc
	type_wrap.cc
	unsigned_literal.cc)

target_link_directories(QAT_AST_PRERUN PUBLIC ../../utils)

target_include_directories(QAT_AST_PRERUN PUBLIC "${LLVM_ROOT_DIR}/include")

target_link_libraries(QAT_AST_PRERUN QAT_UTILS)
