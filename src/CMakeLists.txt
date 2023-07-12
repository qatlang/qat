cmake_minimum_required(VERSION 3.16.3)

PROJECT(qat
	VERSION "0.7.0"
	DESCRIPTION "Qat Programming Language"
	HOMEPAGE_URL "https://qat.dev")

SET(LLVM_ROOT_DIR ${LLVM_DIR})

if(CMAKE_BUILD_TYPE STREQUAL Release)
	SET(IS_RELEASE ON)
	add_compile_definitions(IS_RELEASE=true)
else()
	SET(IS_RELEASE OFF)
	add_compile_definitions(IS_RELEASE=false)
endif()

if(NOT PLATFORM STREQUAL Windows)
	set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
	set(BUILD_SHARED_LIBS OFF)
	set(CMAKE_EXE_LINKER_FLAGS "-static")
endif()

if(PLATFORM STREQUAL Linux)
	add_compile_definitions(PlatformIsLinux=1)
	add_compile_definitions(PlatformIsWindows=0)
	add_compile_definitions(PlatformIsMac=0)
	add_compile_definitions(PlatformIsLinuxArm=0)
	SET(IS_LINUX ON)
elseif(PLATFORM STREQUAL Windows)
	add_compile_definitions(PlatformIsLinux=0)
	add_compile_definitions(PlatformIsWindows=1)
	add_compile_definitions(PlatformIsMac=0)
	add_compile_definitions(PlatformIsLinuxArm=0)
	SET(IS_WINDOWS ON)
elseif(PLATFORM STREQUAL Mac)
	add_compile_definitions(PlatformIsLinux=0)
	add_compile_definitions(PlatformIsWindows=0)
	add_compile_definitions(PlatformIsMac=1)
	add_compile_definitions(PlatformIsLinuxArm=0)
	execute_process(COMMAND brew --prefix llvm@15 OUTPUT_VARIABLE LLVM_ROOT_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
	execute_process(COMMAND brew --prefix zstd OUTPUT_VARIABLE ZSTD_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
	execute_process(COMMAND brew --prefix zlib OUTPUT_VARIABLE ZLIB_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
	execute_process(COMMAND brew --prefix ncurses OUTPUT_VARIABLE NCURSES_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
	SET(IS_MAC ON)
else()
	add_compile_definitions(PlatformIsLinux=0)
	add_compile_definitions(PlatformIsWindows=0)
	add_compile_definitions(PlatformIsMac=0)
	add_compile_definitions(PlatformIsLinuxArm=1)
	SET(IS_LINUX_ARM ON)
endif()

# Setup git related build information
#
#
execute_process(COMMAND git symbolic-ref --short HEAD
	OUTPUT_VARIABLE BUILD_BRANCH)
string(STRIP "${BUILD_BRANCH}" BUILD_BRANCH)
execute_process(COMMAND git log -n 1 --pretty=format:"%H"
	OUTPUT_VARIABLE LAST_COMMIT)
string(STRIP "${LAST_COMMIT}" LAST_COMMIT)

# This sets up compile_commands.json for IDE integration
#
#
SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compile time definitions
#
#
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

# Set C++ Standard
#
#
SET(CMAKE_CXX_STANDARD 20)

# Set Compiler Path
#
#
if(IS_MAC)
	SET(FFI_LIBRARIES /opt/homebrew/opt/libffi/libffi.a)
	SET(ZLIB_LIBRARY /opt/homebrew/opt/zlib/lib/libz.a)
	SET(Terminfo_LIBRARIES /opt/homebrew/opt/ncurses/libcurses.a)
	SET(CMAKE_CXX_COMPILER /usr/bin/clang)
endif()

# Find LLVM package
#
#
if(IS_MAC)
	find_package(LLVM REQUIRED CONFIG PATHS /opt/homebrew/opt/llvm/lib/cmake/llvm NO_DEFAULT_PATH)
else()
	find_package(LLVM REQUIRED CONFIG PATHS ${LLVM_DIR} NO_DEFAULT_PATH)
endif()

message(STATUS "Found LLVM version ${LLVM_VERSION}")
message(STATUS "Using LLVM-Config.cmake from ${LLVM_DIR}")

# Compiler arguments
#
#
if(IS_LINUX)
	if(PLATFORM STREQUAL Arm64)
		if(IS_RELEASE)
			SET(CMAKE_CXX_FLAGS "-static -Os -std=c++20 -fuse-ld=lld -w -s -Wdelete-incomplete -flto -fexceptions -frtti -march=armv7-a -mcpu=cortex-a9 -mfloat-abi=hard --target=arm-linux-gnueabihf")
		else()
			SET(CMAKE_CXX_FLAGS "-static -g -Os -std=c++20 -fuse-ld=lld -Wall -Wdelete-incomplete -fexceptions -frtti -march=armv7-a -mcpu=cortex-a9 -mfloat-abi=hard --target=arm-linux-gnueabihf")
		endif()
	else()
		if(IS_RELEASE)
			SET(CMAKE_CXX_FLAGS "-static -Os -std=c++20 -fuse-ld=lld -w -s -Wdelete-incomplete -flto -fexceptions -frtti")
		else()
			SET(CMAKE_CXX_FLAGS "-static -g -Os -std=c++20 -fuse-ld=lld -Wall -Wdelete-incomplete -fexceptions -frtti")
		endif()
	endif()
elseif(IS_WINDOWS)
	if(IS_RELEASE)
		SET(CMAKE_CXX_FLAGS "/DEBUG:NONE /STATIC /OPT:REF /w /EHa /GR")
	else()
		SET(CMAKE_CXX_FLAGS "/DEBUG:FULL /STATIC /OPT:REF /w /EHa /GR")
	endif()
elseif(IS_MAC)
	if(IS_RELEASE)
		SET(CMAKE_CXX_FLAGS "-static -Os -std=c++20 -fuse-ld=lld -flto -w -s -Wdelete-incomplete -fexceptions -frtti -I/opt/homebrew/opt/llvm/include/")
	else()
		SET(CMAKE_CXX_FLAGS "-static -g -Os -std=c++20 -fuse-ld=lld -Wall -flto -Wdelete-incomplete -fexceptions -frtti -I/opt/homebrew/opt/llvm/include/")
	endif()
endif()

# LLVM Configuration to programmatically link required libs
#
#
if(IS_WINDOWS)
	execute_process(COMMAND ${LLVM_ROOT_DIR}/bin/llvm-config.exe --libs core
		OUTPUT_VARIABLE LLVM_LIBRARIES)
else()
	execute_process(COMMAND ${LLVM_ROOT_DIR}/bin/llvm-config --link-static --libs core
		OUTPUT_VARIABLE LLVM_LIBRARIES)
endif()

string(STRIP "${LLVM_LIBRARIES}" LLVM_LIBRARIES)
message(STATUS "Linking libraries ${LLVM_LIBRARIES}")

# Setup project build
#
#
add_executable(${PROJECT_NAME} qat.cpp qat_sitter.cpp memory_tracker.cpp)
add_subdirectory(cli/)
add_subdirectory(utils/)
add_subdirectory(ast/)
add_subdirectory(lexer/)
add_subdirectory(parser/)
add_subdirectory(IR/)

target_include_directories(${PROJECT_NAME} PUBLIC "${LLVM_ROOT_DIR}/include")

if(IS_WINDOWS)
	target_link_directories(${PROJECT_NAME}
		PUBLIC ast/ lexer/ parser/ utils/ cli/ IR/)
else()
	target_link_directories(${PROJECT_NAME}
		PUBLIC ast/ lexer/ parser/ utils/ cli/ IR/ /usr/local/lib/)
endif()

if(IS_MAC)
	target_link_directories(
		${PROJECT_NAME} PRIVATE
		"${LLVM_ROOT_DIR}/lib"
		"${ZLIB_DIR}/lib"
		"${ZSTD_DIR}/lib"
		"${NCURSES_DIR}/lib"
	)
else()
	target_link_directories(${PROJECT_NAME} PRIVATE ${LLVM_ROOT_DIR}/lib)
endif()

if(IS_LINUX)
	target_link_libraries(
		${PROJECT_NAME} ${LLVM_LIBRARIES}
		LLVMBitWriter LLVMLinker stdc++ stdc++fs rt dl pthread m z tinfo xml2
		QAT_AST QAT_LEXER QAT_PARSER QAT_IR QAT_UTILS QAT_CLI
	)
elseif(IS_MAC)
	target_link_libraries(
		${PROJECT_NAME} ${LLVM_LIBRARIES}
		LLVMBitWriter LLVMLinker stdc++ dl pthread m xml2 z zstd curses
		QAT_AST QAT_LEXER QAT_PARSER QAT_IR QAT_UTILS QAT_CLI
	)
elseif(IS_WINDOWS)
	string(REPLACE " " ";" LLVM_LIBRARIES ${LLVM_LIBRARIES})
	target_link_libraries(
		${PROJECT_NAME}
		QAT_UTILS QAT_CLI QAT_AST QAT_LEXER QAT_PARSER QAT_IR
		${LLVM_LIBRARIES}
		LLVMBitWriter
	)
endif()

if(NOT IS_WINDOWS)
	execute_process(COMMAND "export PATH=${CMAKE_BINARY_DIR}:$PATH")
endif()
