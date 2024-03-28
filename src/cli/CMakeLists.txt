add_library(QAT_CLI
	error.cc
	color.cc
	config.cc
	create.cc
	display.cc
	logger.cc)

target_include_directories(QAT_CLI PUBLIC "${LLVM_ROOT_DIR}/include")

target_link_directories(QAT_CLI PUBLIC ../utils)

target_link_libraries(QAT_CLI QAT_UTILS)