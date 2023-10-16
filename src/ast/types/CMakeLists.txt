add_library(QAT_AST_TYPES
	array.cpp
	c_type.cpp
	float.cpp
	future.cpp
	generic_named_type.cpp
	generic_abstract.cpp
	integer.cpp
	linked_generic.cpp
	maybe.cpp
	named.cpp
	reference.cpp
	pointer.cpp
	prerun_generic.cpp
	qat_type.cpp
	result.cpp
	string_slice.cpp
	tuple.cpp
	typed_generic.cpp
	unsigned.cpp
	void.cpp)

target_include_directories(QAT_AST_TYPES PUBLIC "${LLVM_ROOT_DIR}/include")

target_link_directories(QAT_AST_TYPES PUBLIC ../../utils)

target_link_libraries(QAT_AST_TYPES QAT_UTILS)