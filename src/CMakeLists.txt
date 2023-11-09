cmake_minimum_required(VERSION 3.16.3)

PROJECT(qat
	VERSION "0.7.5"
	DESCRIPTION "Qat Programming Language"
	HOMEPAGE_URL "https://qat.dev")

SET(LLVM_ROOT_DIR ${LLVM_DIR})
SET(CLANG_LINK_CLANG_DYLIB ON)

if(CMAKE_BUILD_TYPE STREQUAL Release)
	SET(IS_RELEASE ON)
	add_compile_definitions(IS_RELEASE=true)
else()
	SET(IS_RELEASE OFF)
	add_compile_definitions(IS_RELEASE=false)
endif()

if(PLATFORM STREQUAL Linux)
	add_compile_definitions(PlatformIsLinux=1)
	add_compile_definitions(PlatformIsWindows=0)
	add_compile_definitions(PlatformIsMac=0)
	SET(IS_LINUX ON)
elseif(PLATFORM STREQUAL Windows)
	add_compile_definitions(PlatformIsLinux=0)
	add_compile_definitions(PlatformIsWindows=1)
	add_compile_definitions(PlatformIsMac=0)
	SET(IS_WINDOWS ON)
else()
	add_compile_definitions(PlatformIsLinux=0)
	add_compile_definitions(PlatformIsWindows=0)
	add_compile_definitions(PlatformIsMac=1)
	execute_process(COMMAND brew --prefix llvm@15 OUTPUT_VARIABLE LLVM_ROOT_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
	execute_process(COMMAND brew --prefix zstd OUTPUT_VARIABLE ZSTD_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
	execute_process(COMMAND brew --prefix zlib OUTPUT_VARIABLE ZLIB_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
	execute_process(COMMAND brew --prefix ncurses OUTPUT_VARIABLE NCURSES_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
	execute_process(COMMAND brew --prefix libffi OUTPUT_VARIABLE LIBFFI_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
	SET(IS_MAC ON)
endif()

execute_process(COMMAND git symbolic-ref --short HEAD
	OUTPUT_VARIABLE BUILD_BRANCH)
string(STRIP "${BUILD_BRANCH}" BUILD_BRANCH)
execute_process(COMMAND git log -n 1 --pretty=format:"%H"
	OUTPUT_VARIABLE LAST_COMMIT)
string(STRIP "${LAST_COMMIT}" LAST_COMMIT)

SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_definitions(QAT_IS_PRERELEASE=false)
add_compile_definitions(QAT_PRERELEASE=alpha)
add_compile_definitions(QAT_BUILD_BRANCH=${BUILD_BRANCH})
add_compile_definitions(QAT_BUILD_ID=${LAST_COMMIT})
add_compile_definitions(QAT_VERSION=${PROJECT_VERSION})

if(IS_RELEASE)
	add_compile_definitions(QAT_BUILD_TYPE=Release)
	add_compile_definitions(NDEBUG=1)
else()
	add_compile_definitions(QAT_BUILD_TYPE=Debug)
	add_compile_definitions(NDEBUG=0)
endif()

SET(CMAKE_CXX_STANDARD 20)

if(IS_MAC)
	SET(FFI_LIBRARIES ${LIBFFI_DIR}/libffi.a)
	SET(ZLIB_LIBRARY ${ZLIB_DIR}/lib/libz.a)
	SET(Terminfo_LIBRARIES ${NCURSES_DIR}/libcurses.a)
endif()

if(IS_MAC OR IS_LINUX)
	find_package(LLVM REQUIRED CONFIG PATHS ${LLVM_ROOT_DIR}/lib/cmake/llvm NO_DEFAULT_PATH)
	find_package(Clang REQUIRED CONFIG PATHS ${LLVM_ROOT_DIR}/lib/cmake/clang NO_DEFAULT_PATH)
else()
	find_package(LLVM REQUIRED CONFIG PATHS ${LLVM_ROOT_DIR} NO_DEFAULT_PATH)
	find_package(Clang REQUIRED CONFIG PATHS ${LLVM_ROOT_DIR} NO_DEFAULT_PATH)
endif()

message(STATUS "Found LLVM version ${LLVM_VERSION}")
message(STATUS "Using LLVM-Config.cmake from ${LLVM_DIR}")

if(IS_LINUX)
	if(IS_RELEASE)
		SET(CMAKE_CXX_FLAGS "-O0 -std=c++20 -fuse-ld=lld -w -s")
	else()
		SET(CMAKE_CXX_FLAGS "-g -O0 -std=c++20 -fuse-ld=lld -Wall -Wunused")
	endif()
elseif(IS_WINDOWS)
	if(IS_RELEASE)
		SET(CMAKE_CXX_FLAGS "/DEBUG:NONE /STATIC /OPT:REF /w /EHa /GR")
	else()
		SET(CMAKE_CXX_FLAGS "/DEBUG:FULL /STATIC /OPT:REF /w /EHa /GR")
	endif()
elseif(IS_MAC)
	if(IS_RELEASE)
		SET(CMAKE_CXX_FLAGS "-shared -Os -std=c++20 -fuse-ld=lld -flto -w -s -Wdelete-incomplete -fPIC -I${LLVM_ROOT_DIR}/include/")
	else()
		SET(CMAKE_CXX_FLAGS "-shared -g -Os -std=c++20 -fuse-ld=lld -Wall -flto -Wdelete-incomplete -fPIC -I${LLVM_ROOT_DIR}/include/")
	endif()
endif()

if(IS_WINDOWS)
	execute_process(COMMAND ${LLVM_ROOT_DIR}/bin/llvm-config.exe --link-static --libs
		OUTPUT_VARIABLE LLVM_LIBRARIES)
elseif(IS_MAC)
	execute_process(COMMAND ${LLVM_ROOT_DIR}/bin/llvm-config --link-shared --libs core
		OUTPUT_VARIABLE LLVM_LIBRARIES)
else()
	execute_process(COMMAND ${LLVM_ROOT_DIR}/bin/llvm-config --link-static --libs
		OUTPUT_VARIABLE LLVM_LIBRARIES)
endif()

string(STRIP "${LLVM_LIBRARIES}" LLVM_LIBRARIES)
message(STATUS "Linking libraries ${LLVM_LIBRARIES}")

add_executable(${PROJECT_NAME} qat.cpp qat_sitter.cpp memory_tracker.cpp)
add_subdirectory(cli/)
add_subdirectory(utils/)
add_subdirectory(ast/)
add_subdirectory(lexer/)
add_subdirectory(parser/)
add_subdirectory(IR/)

target_include_directories(${PROJECT_NAME} PUBLIC "${LLVM_ROOT_DIR}/include")
target_link_directories(${PROJECT_NAME} PUBLIC ast/ lexer/ parser/ utils/ cli/ IR/)

if(IS_MAC)
	target_link_directories(
		${PROJECT_NAME} PRIVATE
		"${LLVM_ROOT_DIR}/lib"
		"${ZLIB_DIR}/lib"
		"${ZSTD_DIR}/lib"
		"${NCURSES_DIR}"
		"${LIBFFI_DIR}"
	)
else()
	target_link_directories(${PROJECT_NAME} PRIVATE ${LLVM_ROOT_DIR}/lib ${LINK_DIRS})
endif()

SET(LLD_LIBRARIES lldCOFF lldCommon lldELF lldMachO lldMinGW lldWasm)

if(IS_LINUX)
	target_link_libraries(
		${PROJECT_NAME} ${LLVM_LIBRARIES} ${LLD_LIBRARIES} clangBasic stdc++ stdc++fs rt pthread m z tinfo xml2
		QAT_AST QAT_LEXER QAT_PARSER QAT_IR QAT_UTILS QAT_CLI
	)
elseif(IS_MAC)
	target_link_libraries(
		${PROJECT_NAME} ${LLVM_LIBRARIES} ${LLD_LIBRARIES} clangBasic
		stdc++ dl pthread m xml2 z zstd curses
		QAT_AST QAT_LEXER QAT_PARSER QAT_IR QAT_UTILS QAT_CLI
	)
elseif(IS_WINDOWS)
	string(REPLACE " " ";" LLVM_LIBRARIES ${LLVM_LIBRARIES})
	target_link_libraries(
		${PROJECT_NAME}
		QAT_UTILS QAT_CLI QAT_AST QAT_LEXER QAT_PARSER QAT_IR
		${LLVM_LIBRARIES} ${LLD_LIBRARIES} clangBasic
	)
endif()

message(STATUS "CMake install prefix is: ${CMAKE_INSTALL_PREFIX}")

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX})
