add_library(QAT_AST STATIC
	bring_bitwidths.cc
	bring_paths.cc
	bring_entities.cc
	convertor.cc
	constructor.cc
	define_choice_type.cc
	define_core_type.cc
	define_mix_type.cc
	define_opaque_type.cc
	define_prerun_function.cc
	define_region.cc
	destructor.cc
	do_skill.cc
	emit_ctx.cc
	function.cc
	generics.cc
	global_declaration.cc
	lib.cc
	method.cc
	mod_info.cc
	node.cc
	operator_function.cc
	sentence.cc
	type_definition.cc)

target_include_directories(QAT_AST PUBLIC "${LLVM_ROOT_DIR}/include")

add_subdirectory(types/)
add_subdirectory(expressions/)
add_subdirectory(sentences/)
add_subdirectory(prerun/)
add_subdirectory(prerun_sentences/)
add_subdirectory(meta/)

target_link_directories(QAT_AST
	PUBLIC ../utils
	PUBLIC ../IR)

target_link_libraries(QAT_AST
	QAT_AST_TYPES
	QAT_AST_EXPRESSIONS
	QAT_AST_SENTENCES
	QAT_AST_PRERUN_SENTENCES
	QAT_AST_PRERUN
	QAT_AST_META
	QAT_UTILS
	QAT_IR)
