add_library(ASTMeta STATIC
  intrinsic.cc
  math.cc
  registers.cc
  todo.cc
)

target_compile_options(ASTMeta PRIVATE
  -Wall
  -Wunused)

target_link_directories(ASTMeta PUBLIC ../../cli)

target_include_directories(ASTMeta SYSTEM PRIVATE "${LLVM_INCLUDE_DIRS}" "${CMAKE_SOURCE_DIR}/../include")

target_link_libraries(ASTMeta cli)
