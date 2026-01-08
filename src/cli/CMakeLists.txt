add_library(cli STATIC
	error.cc
	color.cc
	config.cc
	create.cc
	display.cc
	logger.cc)

target_compile_options(cli PRIVATE
  -Wall
  -Wunused)

target_include_directories(cli SYSTEM PRIVATE "${LLVM_INCLUDE_DIRS}")

target_link_directories(cli PUBLIC ../utils)

target_link_libraries(cli utils)
