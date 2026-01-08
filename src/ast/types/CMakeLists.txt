add_library(ASTTypes STATIC
	address_space.cc
	array.cc
	char.cc
	error.cc
	float.cc
	function.cc
	future.cc
	generic_abstract.cc
	generic_named_type.cc
	generic_integer.cc
	integer.cc
	linked_generic.cc
	maybe.cc
	named.cc
	native_type.cc
	reference.cc
	pointer.cc
	pointer_owner.cc
	polymorph.cc
	prerun_generic.cc
	qat_type.cc
	result.cc
	self_type.cc
	slice.cc
	subtype.cc
	text.cc
	tuple.cc
	typed_generic.cc
	unsigned.cc
	variadics.cc
	vector.cc
	void.cc)

target_compile_options(ASTTypes PRIVATE
  -Wall
  -Wunused)

target_include_directories(ASTTypes SYSTEM PRIVATE "${LLVM_INCLUDE_DIRS}")

target_link_directories(ASTTypes PUBLIC ../../utils)

target_link_libraries(ASTTypes utils)
