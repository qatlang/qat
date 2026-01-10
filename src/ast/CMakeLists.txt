add_library(AST STATIC
	assembly_block.cc
	convertor.cc
	constructor.cc
	define_choice_type.cc
	define_flag_type.cc
	define_mix_type.cc
	define_opaque_type.cc
	define_prerun_function.cc
	define_region.cc
	define_skill.cc
	define_struct_type.cc
	define_toggle_type.cc
	destructor.cc
	do_skill.cc
	emit_ctx.cc
	function.cc
	generics.cc
	global_declaration.cc
	import_bitwidths.cc
	import_paths.cc
	import_entities.cc
	lib.cc
	method.cc
	mod_info.cc
	node.cc
	operator_function.cc
	patterns.cc
	sentence.cc
	skill_entity.cc
	sub_entity_solver.cc
	type_definition.cc
	type_like.cc
)

target_compile_options(AST PRIVATE
  -Wall
  -Wunused)

target_include_directories(AST SYSTEM PRIVATE "${LLVM_INCLUDE_DIRS}" "${CMAKE_SOURCE_DIR}/../include")

add_subdirectory(types/)
add_subdirectory(expressions/)
add_subdirectory(sentences/)
add_subdirectory(prerun/)
add_subdirectory(prerun_sentences/)
add_subdirectory(meta/)

target_link_directories(AST
	PUBLIC ../utils
	PUBLIC ../IR)

target_link_libraries(AST
	ASTTypes
	ASTExpressions
	ASTSentences
	ASTPrerunSentences
	ASTPrerun
	ASTMeta
	utils
	IR)
