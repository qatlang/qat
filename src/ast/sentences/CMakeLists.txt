add_library(QAT_AST_SENTENCES
	assignment.cc
	break.cc
	continue.cc
	expression_sentence.cc
	give_sentence.cc
	if_else.cc
	local_declaration.cc
	loop_infinite.cc
	loop_n_times.cc
	loop_while.cc
	match.cc
	member_initialisation.cc
	say_sentence.cc)

target_include_directories(QAT_AST_SENTENCES PUBLIC "${LLVM_ROOT_DIR}/include")

target_link_directories(QAT_AST_SENTENCES PUBLIC ../../utils)

target_link_libraries(QAT_AST_SENTENCES QAT_UTILS)