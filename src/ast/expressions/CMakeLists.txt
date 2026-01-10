add_library(ASTExpressions STATIC
	address_of.cc
	array_literal.cc
	assembly.cc
	atomic_copy.cc
	atomic_move.cc
	await.cc
	binary_expression.cc
	cast.cc
	confirm_ref.cc
	constructor_call.cc
	copy.cc
	default.cc
	dereference.cc
	end_pointer.cc
	entity.cc
	error.cc
	function_call.cc
	generic_entity.cc
	get_poly.cc
	heap.cc
	in.cc
	inline_let.cc
	inline_match.cc
	is.cc
	is_flag_variant.cc
	is_variant.cc
	method_call.cc
	index_access.cc
	member_access.cc
	move.cc
	negative.cc
	non_null.cc
	not.cc
	ok.cc
	operator.cc
	or_use_value.cc
	plain_initialiser.cc
	self_instance.cc
	sub_entity.cc
	swap.cc
	to_conversion.cc
	tuple_value.cc
	variant_initialiser.cc)

target_compile_options(ASTExpressions PRIVATE
  -Wall
  -Wunused)

target_link_directories(ASTExpressions PUBLIC ../../utils)

target_include_directories(ASTExpressions SYSTEM PRIVATE "${LLVM_INCLUDE_DIRS}" "${CMAKE_SOURCE_DIR}/../include")

target_link_libraries(ASTExpressions utils)
