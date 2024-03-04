add_library(QAT_LEXER lexer.cc token.cc)

target_include_directories(QAT_LEXER PUBLIC "${LLVM_ROOT_DIR}/include")

target_link_directories(QAT_LEXER
	PUBLIC ../cli
	PUBLIC ../utils)

target_link_libraries(QAT_LEXER QAT_CLI QAT_UTILS)