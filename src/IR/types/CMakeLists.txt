add_library(QAT_IR_TYPES STATIC
	array.cc
	choice.cc
	definition.cc
	expanded_type.cc
	float.cc
	function.cc
	future.cc
	integer.cc
	maybe.cc
	native_type.cc
	opaque.cc
	pointer.cc
	polymorph.cc
	qat_type.cc
	reference.cc
	region.cc
	result.cc
	string_slice.cc
	struct_type.cc
	toggle.cc
	tuple.cc
	typed.cc
	mix.cc
	unsigned.cc
	vector.cc
   void.cc)

target_include_directories(QAT_IR_TYPES PUBLIC "${LLVM_ROOT_DIR}/include")

target_link_directories(QAT_IR_TYPES PUBLIC ../../utils)

target_link_libraries(QAT_IR_TYPES QAT_UTILS)
