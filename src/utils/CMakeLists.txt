add_library(QAT_UTILS
	file_range.cpp
	identifier.cpp
	logger.cpp
	parse_boxes_identifier.cpp
	qat_region.cpp
	string_to_callingconv.cpp
	unique_id.cpp
	visibility.cpp
	json_parser.cpp
	json.cpp)

target_include_directories(QAT_UTILS PUBLIC "${LLVM_ROOT_DIR}/include")