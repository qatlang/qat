add_library(QAT_AST
	argument.cpp
	box.cpp
	bring_paths.cpp
	bring_entities.cpp
	convertor.cpp
	constructor.cpp
	define_choice_type.cpp
	define_core_type.cpp
	define_mix_type.cpp
	define_region.cpp
	destructor.cpp
	expression.cpp
	function.cpp
	generics.cpp
	global_declaration.cpp
	lib.cpp
	member_function.cpp
	mod_info.cpp
	node.cpp
	operator_function.cpp
	sentence.cpp
	type_definition.cpp)

target_include_directories(QAT_AST PUBLIC "${LLVM_ROOT_DIR}/include")

add_subdirectory(types/)
add_subdirectory(expressions/)
add_subdirectory(sentences/)
add_subdirectory(prerun/)

target_link_directories(QAT_AST
	PUBLIC ../utils
	PUBLIC ../IR)

target_link_libraries(QAT_AST
	QAT_AST_TYPES
	QAT_AST_EXPRESSIONS
	QAT_AST_SENTENCES
	QAT_AST_CONSTANTS
	QAT_UTILS
	QAT_IR)