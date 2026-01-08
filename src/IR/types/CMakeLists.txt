add_library(IRTypes STATIC
	address_space.cc
	array.cc
	char.cc
	choice.cc
	definition.cc
	error.cc
	expanded_type.cc
	flag.cc
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
	slice.cc
	struct_type.cc
	text.cc
	toggle.cc
	tuple.cc
	typed.cc
	mix.cc
	unsigned.cc
	vector.cc
	void.cc
)

target_compile_options(IRTypes PRIVATE
  -Wall
  -Wunused)

target_include_directories(IRTypes SYSTEM PRIVATE "${LLVM_INCLUDE_DIRS}")

target_link_directories(IRTypes PUBLIC ../../utils)

target_link_libraries(IRTypes utils)
