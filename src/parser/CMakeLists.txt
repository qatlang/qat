add_library(parser STATIC
	parser.cc
	cache_symbol.cc
	parser_context.cc
	token_family.cc)

target_compile_options(parser PRIVATE
  -Wall
  -Wunused)

target_include_directories(parser SYSTEM PRIVATE "${LLVM_INCLUDE_DIRS}")

target_link_directories(parser
	PUBLIC ../ast
	PUBLIC ../lexer
	PUBLIC ../utils
	PUBLIC ../cli)

target_link_libraries(parser AST lexer utils cli)
