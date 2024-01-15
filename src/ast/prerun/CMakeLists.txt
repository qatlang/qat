add_library(QAT_AST_CONSTANTS
	array_literal.cpp
	boolean_literal.cpp
	custom_float_literal.cpp
	custom_integer_literal.cpp
	default.cpp
	entity.cpp
	float_literal.cpp
	integer_literal.cpp
	member_access.cpp
	negative.cpp
	null_pointer.cpp
	plain_initialiser.cpp
	string_literal.cpp
	type_checker.cpp
	type_wrap.cpp
	unsigned_literal.cpp)

target_link_directories(QAT_AST_CONSTANTS PUBLIC ../../utils)

target_include_directories(QAT_AST_CONSTANTS PUBLIC "${LLVM_ROOT_DIR}/include")

target_link_libraries(QAT_AST_CONSTANTS QAT_UTILS)