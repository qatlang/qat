add_library(QAT_LEXER lexer.cpp token.cpp)
target_link_libraries(QAT_LEXER QAT_CLI QAT_UTILITIES)