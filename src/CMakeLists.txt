cmake_minimum_required(VERSION 3.16.3)

PROJECT(qat
	VERSION "0.8.0"
	DESCRIPTION "Qat Programming Language"
	HOMEPAGE_URL "https://qat.dev")

set(LLVM_ROOT_DIR ${LLVM_DIR})
set(Boost_INCLUDE_DIR ${BOOST_INCLUDE_DIR})

if(CMAKE_BUILD_TYPE STREQUAL Release)
	set(IS_RELEASE ON)
	add_compile_definitions(IS_RELEASE=true)
else()
	set(IS_RELEASE OFF)
	add_compile_definitions(IS_RELEASE=false)
endif()

if(PLATFORM STREQUAL Linux)
	add_compile_definitions(PlatformIsLinux=1)
	add_compile_definitions(PlatformIsWindows=0)
	add_compile_definitions(PlatformIsMac=0)
	set(IS_LINUX ON)
elseif(PLATFORM STREQUAL Windows)
	add_compile_definitions(PlatformIsLinux=0)
	add_compile_definitions(PlatformIsWindows=1)
	add_compile_definitions(PlatformIsMac=0)
	set(IS_WINDOWS ON)
elseif(PLATFORM STREQUAL Mac)
	add_compile_definitions(PlatformIsLinux=0)
	add_compile_definitions(PlatformIsWindows=0)
	add_compile_definitions(PlatformIsMac=1)
	set(IS_MAC ON)
endif()

execute_process(COMMAND git symbolic-ref --short HEAD
	OUTPUT_VARIABLE BUILD_BRANCH)
string(STRIP "${BUILD_BRANCH}" BUILD_BRANCH)

execute_process(COMMAND git log -n 1 --pretty=format:"%H"
	OUTPUT_VARIABLE LAST_COMMIT)
string(STRIP "${LAST_COMMIT}" LAST_COMMIT)

execute_process(COMMAND git status
	OUTPUT_VARIABLE GIT_STATUS)
string(STRIP "${GIT_STATUS}" GIT_STATUS)
string(FIND "${GIT_STATUS}" "nothing to commit" NO_CHANGES_AFTER_COMMIT)

if(NO_CHANGES_AFTER_COMMIT EQUAL -1)
	add_compile_definitions(QAT_GIT_HAS_CHANGES=true)
else()
	add_compile_definitions(QAT_GIT_HAS_CHANGES=false)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_definitions(QAT_IS_PRERELEASE=false)
add_compile_definitions(QAT_PRERELEASE=alpha)
add_compile_definitions(QAT_BUILD_BRANCH=${BUILD_BRANCH})
add_compile_definitions(QAT_BUILD_ID=${LAST_COMMIT})
add_compile_definitions(QAT_VERSION=${PROJECT_VERSION})
add_compile_definitions(LLVM_DISABLE_ABI_BREAKING_CHECKS_ENFORCING=1)

if(IS_RELEASE)
	add_compile_definitions(QAT_BUILD_TYPE=Release)
	add_compile_definitions(NDEBUG=1)
else()
	add_compile_definitions(QAT_BUILD_TYPE=Debug)
	add_compile_definitions(NDEBUG=0)
endif()

set(CMAKE_CXX_STANDARD 20)

if(IS_WINDOWS)
	find_package(LLVM REQUIRED CONFIG PATHS ${LLVM_ROOT_DIR} NO_DEFAULT_PATH)
	find_package(Clang REQUIRED CONFIG PATHS ${LLVM_ROOT_DIR} NO_DEFAULT_PATH)
else()
	find_package(LLVM REQUIRED CONFIG PATHS ${LLVM_ROOT_DIR}/lib/cmake/llvm NO_DEFAULT_PATH)
	find_package(Clang REQUIRED CONFIG PATHS ${LLVM_ROOT_DIR}/lib/cmake/clang NO_DEFAULT_PATH)
endif()

message(STATUS "Found LLVM version ${LLVM_VERSION}")
message(STATUS "Using LLVM-Config.cmake from ${LLVM_DIR}")

if(IS_LINUX)
	if(IS_RELEASE)
		set(CMAKE_CXX_FLAGS "-O2 -std=c++20 -fuse-ld=lld -flto -w -s -I${LLVM_ROOT_DIR}/include -funwind-tables -fno-rtti -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS")
	else()
		set(CMAKE_CXX_FLAGS "-g -O0 -std=c++20 -fuse-ld=lld -Wall -Wunused -I${LLVM_ROOT_DIR}/include -funwind-tables -fno-rtti -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS")
	endif()
elseif(IS_MAC)
	set(CMAKE_CXX_FLAGS "-std=c++20 -fuse-ld=lld -flto -w -s -I${LLVM_ROOT_DIR}/include -funwind-tables -fno-rtti")
elseif(IS_WINDOWS)
	if(IS_RELEASE)
		set(CMAKE_CXX_FLAGS "/DEBUG:NONE /STD:c++20 /SUBSYSTEM:CONSOLE /STATIC /OPT:REF /w /EHa /GR /EHsc")
	else()
		set(CMAKE_CXX_FLAGS "/DEBUG:FULL /STD:c++20 /SUBSYSTEM:CONSOLE /STATIC /OPT:REF /w /EHa /GR /EHsc")
	endif()
endif()

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost COMPONENTS filesystem REQUIRED CONFIG PATHS "${BOOST_LINK_DIR}/cmake" NO_DEFAULT_PATH)

if(IS_WINDOWS)
	execute_process(COMMAND ${LLVM_ROOT_DIR}/bin/llvm-config.exe --link-static --libs
		OUTPUT_VARIABLE LLVM_LIBRARIES)
elseif(IS_LINUX)
	execute_process(COMMAND ${LLVM_ROOT_DIR}/bin/llvm-config --link-static --libs
		OUTPUT_VARIABLE LLVM_LIBRARIES)
elseif(IS_MAC)
	execute_process(COMMAND llvm-config --link-static --libs
		OUTPUT_VARIABLE LLVM_LIBRARIES)
endif()

string(STRIP "${LLVM_LIBRARIES}" LLVM_LIBRARIES)

add_executable(${PROJECT_NAME} qat.cc sitter.cc)
add_subdirectory(cli/)
add_subdirectory(utils/)
add_subdirectory(ast/)
add_subdirectory(lexer/)
add_subdirectory(parser/)
add_subdirectory(IR/)

target_include_directories(${PROJECT_NAME} PUBLIC "${LLVM_ROOT_DIR}/include" "${BOOST_INCLUDE_DIR}")
target_link_directories(${PROJECT_NAME}
	PUBLIC ast/ lexer/ parser/ utils/ cli/ IR/
	PRIVATE ${LLVM_ROOT_DIR}/lib ${LINK_DIRS} ${BOOST_LINK_DIR} "${CMAKE_SOURCE_DIR}/../${SHARED_LIBPATH}")

set(LLD_LIBRARIES lldCOFF lldCommon lldELF lldMachO lldMinGW lldWasm)

if(IS_LINUX)
	message(STATUS "Linking libraries ${LLVM_LIBRARIES}")
	message("BOOST libs are: ${Boost_LIBRARIES}")
	target_link_libraries(
		${PROJECT_NAME} ${LLVM_LIBRARIES} ${LLD_LIBRARIES} clangBasic stdc++ rt pthread m z tinfo xml2 icuuc icudata lzma
		QAT_AST QAT_LEXER QAT_PARSER QAT_IR QAT_UTILS QAT_CLI ${Boost_LIBRARIES}
	)
elseif(IS_MAC)
	set(LLVM_LIB_PATHS "")
	string(REPLACE " " ";" LLVM_LIBRARIES "${LLVM_LIBRARIES}")

	foreach(llvmLib ${LLVM_LIBRARIES})
		string(REPLACE "-l" "" libName "${llvmLib}")
		list(APPEND LLVM_LIB_PATHS "${LLVM_ROOT_DIR}/lib/lib${libName}.a")
	endforeach()

	set(LLD_LIB_PATHS "")
	string(REPLACE " " ";" LLD_LIBRARIES "${LLD_LIBRARIES}")

	foreach(lldLib ${LLD_LIBRARIES})
		string(REPLACE "-l" "" libName "${lldLib}")
		list(APPEND LLD_LIB_PATHS "${LLVM_ROOT_DIR}/lib/lib${libName}.a")
	endforeach()

	set(LLVM_LIB_PATHS ${LLVM_LIB_PATHS})
	message(STATUS "LLVM Libraries are: ${LLVM_LIB_PATHS}")
	message(STATUS "LLD Libraries are: ${LLD_LIB_PATHS}")
	message(STATUS "BOOST libs are: ${Boost_LIBRARIES}")
	target_link_libraries(${PROJECT_NAME}
		${LLVM_LIB_PATHS} ${LLD_LIB_PATHS} clangBasic pthread
		"${ZLIB_DIR}/lib/libz.a" "${ZSTD_DIR}/lib/libzstd.a" "${OPENLIBM_DIR}/lib/libopenlibm.a"
		"${NCURSES_DIR}/lib/libcurses.a" "${XML2_DIR}/lib/libxml2.dylib" "${ICONV_DIR}/lib/libiconv.a"
		"${ICU4C_DIR}/lib/libicui18n.a" "${ICU4C_DIR}/lib/libicuio.a" "${ICU4C_DIR}/lib/libicuuc.a"
		"${ICU4C_DIR}/lib/libicudata.a" "${ICU4C_DIR}/lib/libicutu.a" "${LZMA_DIR}/lib/liblzma.a"
		QAT_AST QAT_LEXER QAT_PARSER QAT_IR QAT_UTILS QAT_CLI ${Boost_LIBRARIES}
	)
elseif(IS_WINDOWS)
	string(REPLACE " " ";" LLVM_LIBRARIES "${LLVM_LIBRARIES}")
	message(STATUS "Linking libraries ${LLVM_LIBRARIES}")
	message(STATUS "BOOST libs are: ${Boost_LIBRARIES}")
	target_link_libraries(${PROJECT_NAME}
		QAT_UTILS QAT_CLI QAT_AST QAT_LEXER QAT_PARSER QAT_IR
		${LLVM_LIBRARIES} ${LLD_LIBRARIES} clangBasic ${Boost_LIBRARIES}
	)
endif()

message(STATUS "CMake install prefix is: ${CMAKE_INSTALL_PREFIX}")

include(GNUInstallDirs)

install(TARGETS "${PROJECT_NAME}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

install(FILES "${CLANG_EXE_PATH}" TYPE "BIN" RENAME "clang-17"
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

install(FILES "${LLVM_ROOT_DIR}/bin/llvm-ar" TYPE "BIN" RENAME "llvm-ar-17"
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

install(FILES "${LLVM_ROOT_DIR}/bin/lld" TYPE "BIN"
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

install(DIRECTORY "${CMAKE_SOURCE_DIR}/std/" DESTINATION "std")
install(DIRECTORY DESTINATION "toolchain")

set(CPACK_GENERATOR "ZIP")
set(CPACK_VERBATIM_VARIABLES YES)
string(TOLOWER "${CMAKE_SYSTEM_NAME}" SYSTEM_NAME_LOWERCASE)
string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" SYSTEM_PROCESSOR_LOWERCASE)
set(BUNDLE_FILE_NAME "qat-${PROJECT_VERSION}-${SYSTEM_NAME_LOWERCASE}-${SYSTEM_PROCESSOR_LOWERCASE}")
file(WRITE "${CMAKE_SOURCE_DIR}/../bundle.txt" "${BUNDLE_FILE_NAME}")
set(CPACK_ARCHIVE_FILE_NAME "${BUNDLE_FILE_NAME}")
include(CPack)