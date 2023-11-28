add_library(QAT_AST_CONSTANTS
	boolean_literal.cpp
	custom_float_literal.cpp
	custom_integer_literal.cpp
	default.cpp
	entity.cpp
	float_literal.cpp
	integer_literal.cpp
	negative.cpp
	null_pointer.cpp
	string_literal.cpp
	size_of_type.cpp
	type_checker.cpp
	unsigned_literal.cpp)

target_link_directories(QAT_AST_CONSTANTS PUBLIC ../../utils)

target_include_directories(QAT_AST_CONSTANTS PUBLIC "${LLVM_ROOT_DIR}/include")

target_link_libraries(QAT_AST_CONSTANTS QAT_UTILS)