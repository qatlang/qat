add_library(lexer STATIC
  lexer.cc)

target_compile_options(lexer PRIVATE
  -Wall
  -Wunused)

target_include_directories(lexer SYSTEM PRIVATE "${LLVM_INCLUDE_DIRS}")

target_link_directories(lexer
	PUBLIC ../cli
	PUBLIC ../utils)

target_link_libraries(lexer cli utils)
