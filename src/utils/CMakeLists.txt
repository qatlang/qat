add_library(QAT_UTILS
	file_range.cpp
	get_directory_contents.cpp
	get_parent_names.cpp
	identifier.cpp
	is_integer.cpp
	logger.cpp
	number_to_position.cpp
	parse_boxes_identifier.cpp
	qat_region.cpp
	random_number.cpp
	split_string.cpp
	string_to_callingconv.cpp
	unique_id.cpp
	version_number.cpp
	visibility.cpp
	json_parser.cpp
	json.cpp)

target_include_directories(QAT_UTILS PUBLIC "${LLVM_ROOT_DIR}/include")