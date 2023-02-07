cmake_minimum_required(VERSION 3.16.3)

PROJECT(qat
	VERSION "0.7.0"
	DESCRIPTION "Qat Programming Language"
	HOMEPAGE_URL "https://qat.dev")

if(CMAKE_BUILD_TYPE STREQUAL Release)
	SET(IS_RELEASE ON)
	add_compile_definitions(IS_RELEASE=true)
else()
	SET(IS_RELEASE OFF)
	add_compile_definitions(IS_RELEASE=false)
endif()

set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_EXE_LINKER_FLAGS "-static")

if(PLATFORM STREQUAL Windows)
	SET(IS_WINDOWS ON)
	add_compile_definitions(PLATFORM_IS_LINUX=0)
	add_compile_definitions(PLATFORM_IS_WINDOWS=1)
	add_compile_definitions(PLATFORM_IS_ARMv7=0)
	add_compile_definitions(PLATFORM_IS_MAC=0)
elseif(PLATFORM STREQUAL Arm64)
	SET(IS_LINUX ON)
	add_compile_definitions(PLATFORM_IS_LINUX=0)
	add_compile_definitions(PLATFORM_IS_WINDOWS=0)
	add_compile_definitions(PLATFORM_IS_ARMv7=1)
	add_compile_definitions(PLATFORM_IS_MAC=0)
elseif(PLATFORM STREQUAL Mac)
	SET(IS_MAC ON)
	add_compile_definitions(PLATFORM_IS_LINUX=0)
	add_compile_definitions(PLATFORM_IS_WINDOWS=0)
	add_compile_definitions(PLATFORM_IS_ARMv7=0)
	add_compile_definitions(PLATFORM_IS_MAC=1)
else()
	SET(IS_LINUX ON)
	add_compile_definitions(PLATFORM_IS_LINUX=1)
	add_compile_definitions(PLATFORM_IS_ARMv7=0)
	add_compile_definitions(PLATFORM_IS_WINDOWS=0)
	add_compile_definitions(PLATFORM_IS_MAC=0)
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
# SET(CMAKE_CXX_STANDARD 20)

# Set Compiler Path
#
#
if(IS_LINUX)
	SET(CMAKE_CXX_COMPILER /usr/lib/llvm-15/bin/clang)
elseif(IS_WINDOWS)
	SET(CMAKE_CXX_COMPILER C:\\msys64\\mingw64\\bin\\clang++)
elseif(IS_MAC)
	SET(FFI_LIBRARIES /opt/homebrew/opt/libffi/libffi.a)
	SET(ZLIB_LIBRARY /opt/homebrew/opt/zlib/lib/libz.a)
	SET(Terminfo_LIBRARIES /opt/homebrew/opt/ncurses/libcurses.a)
	SET(CMAKE_CXX_COMPILER /usr/bin/clang)
endif()

# Find LLVM package
#
#
if(IS_LINUX)
	if(IS_RELEASE)
		find_package(LLVM REQUIRED CONFIG PATHS /mnt/Main/LIBS/llvm-release NO_DEFAULT_PATH)
	else()
		find_package(LLVM REQUIRED CONFIG PATHS /mnt/Main/LIBS/llvm-debug NO_DEFAULT_PATH)
	endif()
elseif(IS_WINDOWS)
	find_package(LLVM REQUIRED CONFIG PATHS C:\\msys64\\mingw64\\lib\\cmake\\llvm NO_DEFAULT_PATH)
elseif(IS_MAC)
	find_package(LLVM REQUIRED CONFIG PATHS /opt/homebrew/opt/llvm/lib/cmake/llvm NO_DEFAULT_PATH)
endif()

message(STATUS "Found LLVM version ${LLVM_VERSION}")
message(STATUS "Using LLVM-Config.cmake from ${LLVM_DIR}")

# Compiler arguments
#
#
if(IS_LINUX)
	if(PLATFORM STREQUAL Arm64)
		if(IS_RELEASE)
			SET(CMAKE_CXX_FLAGS "-static -Os -std=c++20 -fuse-ld=lld -w -s -Wdelete-incomplete -flto -fexceptions -frtti -I/mnt/Main/LIBS/llvm-release/include/ -march=armv7-a -mcpu=cortex-a9 -mfloat-abi=hard --target=arm-linux-gnueabihf")
		else()
			SET(CMAKE_CXX_FLAGS "-static -g -Os -std=c++20 -fuse-ld=lld -Wall -Wdelete-incomplete -fexceptions -frtti -I/mnt/Main/LIBS/llvm-debug/include/ -march=armv7-a -mcpu=cortex-a9 -mfloat-abi=hard --target=arm-linux-gnueabihf")
		endif()
	else()
		if(IS_RELEASE)
			SET(CMAKE_CXX_FLAGS "-static -Os -std=c++20 -fuse-ld=lld -w -s -Wdelete-incomplete -flto -fexceptions -frtti -I/usr/include/c++/12 -I/mnt/Main/LIBS/llvm-release/include/")
		else()
			SET(CMAKE_CXX_FLAGS "-static -g -Os -std=c++20 -fuse-ld=lld -Wall -Wdelete-incomplete -fexceptions -frtti -I/usr/include/c++/12 -I/mnt/Main/LIBS/llvm-debug/include/")
		endif()
	endif()
elseif(IS_WINDOWS)
	if(IS_RELEASE)
		SET(CMAKE_CXX_FLAGS "-static -Oz -std=c++20 -fuse-ld=lld -flto -w -s -Wdelete-incomplete -fexceptions -frtti -IC:\\msys64\\mingw64\\include")
	else()
		SET(CMAKE_CXX_FLAGS "-static -g -Oz -std=c++20 -fuse-ld=lld -Wall -flto -Wdelete-incomplete -fexceptions -frtti -IC:\\msys64\\mingw64\\include")
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
if(IS_LINUX)
	if(IS_RELEASE)
		execute_process(COMMAND /mnt/Main/LIBS/llvm-release/bin/llvm-config --link-static --libs core
			OUTPUT_VARIABLE LLVM_LIBRARIES)
	else()
		execute_process(COMMAND /mnt/Main/LIBS/llvm-debug/bin/llvm-config --link-static --libs core
			OUTPUT_VARIABLE LLVM_LIBRARIES)
	endif()
else()
	execute_process(COMMAND llvm-config --link-static --libs core
		OUTPUT_VARIABLE LLVM_LIBRARIES)
endif()

string(STRIP "${LLVM_LIBRARIES}" LLVM_LIBRARIES)
message(STATUS "Linking libraries ${LLVM_LIBRARIES}")

# Setup project build
#
#
add_executable(${PROJECT_NAME} qat.cpp qat_sitter.cpp memory_tracker.cpp)
add_subdirectory(ast/)
add_subdirectory(lexer/)
add_subdirectory(parser/)
add_subdirectory(utils/)
add_subdirectory(IR/)
add_subdirectory(cli/)
target_link_directories(${PROJECT_NAME}
	PRIVATE ast/
	PRIVATE lexer/
	PRIVATE parser/
	PRIVATE utils/
	PRIVATE cli/
	PRIVATE IR/
	PRIVATE /usr/local/lib/
)

if(IS_LINUX)
	if(IS_RELEASE)
		target_link_directories(${PROJECT_NAME} PRIVATE /mnt/Main/LIBS/llvm-release/lib)
	else()
		target_link_directories(${PROJECT_NAME} PRIVATE /mnt/Main/LIBS/llvm-debug/lib)
	endif()
elseif(IS_MAC)
	target_link_directories(
		${PROJECT_NAME} PRIVATE
		/opt/homebrew/opt/llvm/lib
		/opt/homebrew/opt/zlib/lib
		/opt/homebrew/opt/zstd/lib
	)
elseif(IS_WINDOWS)
	target_link_directories(${PROJECT_NAME} PRIVATE C:\\msys64\\mingw64\\lib)

	# SET(FFI_LIBRARIES /c/msys64/mingw64/lib/libffi.a)
	# SET(ZLIB_LIBRARY /c/msys64/mingw64/lib/libz.a)
	# SET(LIBXML2_LIBRARY /c/msys64/mingw64/lib/libxml2.a)
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
	target_link_libraries(
		${PROJECT_NAME} ${LLVM_LIBRARIES}
		LLVMBitWriter stdc++ pthread m z xml2
		QAT_AST QAT_LEXER QAT_PARSER QAT_IR QAT_UTILS QAT_CLI
	)
endif()

if(IS_LINUX OR IS_MAC)
	execute_process(COMMAND "export PATH=${CMAKE_BINARY_DIR}:$PATH")
endif()