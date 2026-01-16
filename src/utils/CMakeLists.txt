add_library(utils STATIC
	file_range.cpp
	identifier.cpp
	profiler.cc
	qat_region.cpp
	run_command.cc
	string_to_callingconv.cpp
	visibility.cpp
	utils.cc
)

target_compile_options(utils PRIVATE
  -Wall
  -Wunused)

target_include_directories(utils SYSTEM PRIVATE "${LLVM_INCLUDE_DIRS}" "${Boost_INCLUDE_DIRS}" "${LIBS_DIR}/icu4c/include" "${CMAKE_SOURCE_DIR}/../include")

target_link_libraries(utils PUBLIC Boost::filesystem Boost::json Boost::process)
