add_library(QAT_AST_TYPES
	array.cpp
	cstring.cpp
	const_generic.cpp
	float.cpp
	future.cpp
	integer.cpp
	maybe.cpp
	named.cpp
	reference.cpp
	pointer.cpp
	qat_type.cpp
	string_slice.cpp
	generic_named_type.cpp
	generic_abstract.cpp
	tuple.cpp
	typed_generic.cpp
	unsigned.cpp
	void.cpp)

target_link_libraries(QAT_AST_TYPES QAT_UTILS)