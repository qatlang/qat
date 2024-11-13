add_library(QAT_AST_META todo.cc)
target_link_directories(QAT_AST_META PUBLIC ../../cli)
target_include_directories(QAT_AST_META PUBLIC "${LLVM_ROOT_DIR}/include")
target_link_libraries(QAT_AST_META QAT_CLI)
