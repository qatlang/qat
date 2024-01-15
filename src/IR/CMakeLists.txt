add_library(QAT_IR
	context.cpp
	generics.cpp
	logic.cpp
	entity_overview.cpp
	function.cpp
	global_entity.cpp
	link_names.cpp
	member_function.cpp
	control_flow.cpp
	qat_module.cpp
	skill.cpp
	static_member.cpp
	stdlib.cpp
	use.cpp
	value.cpp)

target_include_directories(QAT_IR PUBLIC "${LLVM_ROOT_DIR}/include")

add_subdirectory(types/)

target_link_directories(QAT_IR
	PUBLIC ../lexer
	PUBLIC ../utils
	PUBLIC ../ast)

target_link_libraries(QAT_IR QAT_LEXER QAT_UTILS QAT_AST QAT_IR_TYPES)