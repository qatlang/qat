add_library(QAT_UTILS
	file_range.cpp
	identifier.cpp
	json_parser.cpp
	json.cpp
	parse_boxes_identifier.cpp
	qat_region.cpp
	run_command.cc
	string_to_callingconv.cpp
	visibility.cpp
	utils.cc
)

target_include_directories(QAT_UTILS PUBLIC "${LLVM_INCLUDE_DIRS}" "${Boost_INCLUDE_DIRS}" "${LIBS_DIR}/icu4c/include")

target_link_libraries(QAT_UTILS PUBLIC Boost::filesystem)
