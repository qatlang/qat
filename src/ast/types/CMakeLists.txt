add_library(QAT_AST_TYPES
	array.cc
	c_type.cc
	float.cc
	function.cc
	future.cc
	generic_named_type.cc
	generic_abstract.cc
	integer.cc
	linked_generic.cc
	maybe.cc
	named.cc
	reference.cc
	pointer.cc
	prerun_generic.cc
	qat_type.cc
	result.cc
	self_type.cc
	string_slice.cc
	tuple.cc
	typed_generic.cc
	unsigned.cc
	vector.cc
	void.cc)

target_include_directories(QAT_AST_TYPES PUBLIC "${LLVM_ROOT_DIR}/include")

target_link_directories(QAT_AST_TYPES PUBLIC ../../utils)

target_link_libraries(QAT_AST_TYPES QAT_UTILS)