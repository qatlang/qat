add_library(QAT_AST_META
  intrinsic.cc
  math.cc
  registers.cc
  todo.cc
)
target_link_directories(QAT_AST_META PUBLIC ../../cli)
target_include_directories(QAT_AST_META PUBLIC "${LLVM_INCLUDE_DIRS}")
target_link_libraries(QAT_AST_META QAT_CLI)
