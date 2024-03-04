add_library(QAT_PARSER
	parser.cc
	cache_symbol.cc
	parser_context.cc
	token_family.cc)

target_include_directories(QAT_PARSER PUBLIC "${LLVM_ROOT_DIR}/include")

target_link_directories(QAT_PARSER
	PUBLIC ../ast
	PUBLIC ../lexer
	PUBLIC ../utils
	PUBLIC ../cli)

target_link_libraries(QAT_PARSER QAT_AST QAT_LEXER QAT_UTILS QAT_CLI)