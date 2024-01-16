add_library(QAT_AST_SENTENCES
	assignment.cpp
	break.cpp
	continue.cpp
	expression_sentence.cpp
	give_sentence.cpp
	if_else.cpp
	local_declaration.cpp
	loop_infinite.cpp
	loop_n_times.cpp
	loop_while.cpp
	match.cpp
	member_initialisation.cpp
	say_sentence.cpp)

target_include_directories(QAT_AST_SENTENCES PUBLIC "${LLVM_ROOT_DIR}/include")

target_link_directories(QAT_AST_SENTENCES PUBLIC ../../utils)

target_link_libraries(QAT_AST_SENTENCES QAT_UTILS)