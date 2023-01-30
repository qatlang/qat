add_library(QAT_IR_TYPES
	array.cpp
	choice.cpp
	core_type.cpp
	cstring.cpp
	definition.cpp
	expanded_type.cpp
	float.cpp
	function.cpp
	future.cpp
	integer.cpp
	maybe.cpp
	opaque.cpp
	pointer.cpp
	qat_type.cpp
	reference.cpp
	region.cpp
	string_slice.cpp
	tuple.cpp
	typed.cpp
	mix.cpp
	unsigned.cpp
	void.cpp)

target_link_libraries(QAT_IR_TYPES QAT_UTILS)