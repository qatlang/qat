add_library(QAT_UTILS
	file_range.cpp
	identifier.cpp
	parse_boxes_identifier.cpp
	qat_region.cpp
	run_command.cc
	string_to_callingconv.cpp
	unique_id.cpp
	visibility.cpp
	json_parser.cpp
	json.cpp)

target_include_directories(QAT_UTILS PUBLIC "${LLVM_ROOT_DIR}/include" "${BOOST_INCLUDE_DIR}")

target_link_libraries(QAT_UTILS PUBLIC)