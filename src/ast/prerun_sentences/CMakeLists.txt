add_library(QAT_AST_PRERUN_SENTENCES
  break.cc
  continue.cc
  expression_sentence.cc
  give_sentence.cc
  loop_to.cc
  prerun_sentence.cc
  say.cc
)

target_include_directories(QAT_AST_SENTENCES PUBLIC "${LLVM_ROOT_DIR}/include")

target_link_directories(QAT_AST_SENTENCES PUBLIC ../../utils)

target_link_libraries(QAT_AST_PRERUN_SENTENCES PUBLIC QAT_UTILS QAT_IR)
