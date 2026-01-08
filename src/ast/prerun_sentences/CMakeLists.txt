add_library(ASTPrerunSentences STATIC
  break.cc
  continue.cc
  expression_sentence.cc
  give_sentence.cc
  if_else.cc
  loop_to.cc
  say.cc
)

target_compile_options(ASTPrerunSentences PRIVATE
  -Wall
  -Wunused)

target_include_directories(ASTPrerunSentences SYSTEM PRIVATE "${LLVM_INCLUDE_DIRS}")

target_link_directories(ASTPrerunSentences PUBLIC ../../utils)

target_link_libraries(ASTPrerunSentences PUBLIC utils IR)
