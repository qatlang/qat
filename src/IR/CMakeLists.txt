add_library(IR STATIC
	context.cc
	control_flow.cc
	generics.cc
	internal.cc
	logic.cc
	function.cc
	global_entity.cc
	link_names.cc
	metalib.cc
	method.cc
	prerun_function.cc
	qat_module.cc
	skill.cc
	static_member.cc
	stdlib.cc
	type_id.cc
	value.cc)

target_compile_options(IR PRIVATE
  -Wall
  -Wunused)

target_include_directories(IR SYSTEM PRIVATE "${LLVM_INCLUDE_DIRS}" "${Boost_INCLUDE_DIRS}" "${CMAKE_SOURCE_DIR}/../include")

add_subdirectory(types/)

target_link_directories(IR
	PUBLIC ../lexer
	PUBLIC ../utils
	PUBLIC ../ast)

target_link_libraries(IR lexer utils AST IRTypes ASTPrerunSentences Boost::json)
