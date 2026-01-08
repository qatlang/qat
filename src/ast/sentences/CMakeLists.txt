add_library(ASTSentences STATIC
	assignment.cc
	break.cc
	continue.cc
	expression_sentence.cc
	give_sentence.cc
	if_else.cc
	ignore_value.cc
	local_declaration.cc
	loop_infinite.cc
	loop_if.cc
	loop_in.cc
	loop_to.cc
	match.cc
	member_initialisation.cc
	say_sentence.cc
	use_declaration.cc
)

target_compile_options(ASTSentences PRIVATE
  -Wall
  -Wunused)

target_include_directories(ASTSentences SYSTEM PRIVATE "${LLVM_INCLUDE_DIRS}")

target_link_directories(ASTSentences PUBLIC ../../utils)

target_link_libraries(ASTSentences utils)
