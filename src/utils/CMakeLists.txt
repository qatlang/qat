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

target_include_directories(QAT_UTILS PUBLIC "${CMAKE_SOURCE_DIR}/../include" "${LLVM_ROOT_DIR}/include" "${BOOST_DIR}/include")

target_link_libraries(QAT_UTILS PUBLIC Boost::filesystem)
