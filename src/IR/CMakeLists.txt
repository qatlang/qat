add_library(QAT_IR STATIC
	context.cc
	control_flow.cc
	generics.cc
	internal.cc
	logic.cc
	entity_overview.cc
	function.cc
	global_entity.cc
	link_names.cc
	method.cc
	prerun_function.cc
	qat_module.cc
	skill.cc
	static_member.cc
	stdlib.cc
	value.cc)

target_include_directories(QAT_IR PUBLIC "${LLVM_ROOT_DIR}/include" "${BOOST_INCLUDE_DIR}")

add_subdirectory(types/)

target_link_directories(QAT_IR
	PUBLIC ../lexer
	PUBLIC ../utils
	PUBLIC ../ast)

target_link_libraries(QAT_IR QAT_LEXER QAT_UTILS QAT_AST QAT_IR_TYPES QAT_AST_PRERUN_SENTENCES)
