cmake_minimum_required(VERSION 3.16.3)

PROJECT(qat
    VERSION "0.5.0"
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

if(WIN32)
    SET(IS_UNIX OFF)
    add_compile_definitions(PLATFORM_IS_UNIX=0)
    add_compile_definitions(PLATFORM_IS_WINDOWS=1)
else()
    SET(IS_UNIX ON)
    add_compile_definitions(PLATFORM_IS_UNIX=1)
    add_compile_definitions(PLATFORM_IS_WINDOWS=0)
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
add_compile_definitions(QAT_IS_PRERELEASE=true)
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
if(IS_UNIX)
    SET(CMAKE_CXX_COMPILER /usr/lib/llvm-15/bin/clang)
else()
    SET(CMAKE_CXX_COMPILER C:\\msys64\\mingw64\\bin\\clang++)
endif()

# Find LLVM package
#
#
if(IS_UNIX)
    if(IS_RELEASE)
        find_package(LLVM REQUIRED CONFIG PATHS /mnt/Main/LIBS/llvm-release NO_DEFAULT_PATH)
    else()
        find_package(LLVM REQUIRED CONFIG PATHS /mnt/Main/LIBS/llvm-debug NO_DEFAULT_PATH)
    endif()
else()
    find_package(LLVM REQUIRED CONFIG PATHS C:\\msys64\\mingw64\\lib\\cmake\\llvm NO_DEFAULT_PATH)
endif()

message(STATUS "Found LLVM version ${LLVM_VERSION}")
message(STATUS "Using LLVM-Config.cmake from ${LLVM_DIR}")

# Compiler Flags for different build types
# -IC:\\msys64\\mingw64\\include\\c++\\12.1.0\\x86_64-w64-mingw32
#
if(IS_UNIX)
    if(IS_RELEASE)
        SET(CMAKE_CXX_FLAGS "-static -Os -std=c++20 -fuse-ld=lld -flto -w -s -Wdelete-incomplete -fexceptions -frtti -I/mnt/Main/LIBS/llvm-release/include/")
    else()
        SET(CMAKE_CXX_FLAGS "-static -g -Os -std=c++20 -fuse-ld=lld -Wall -flto -Wdelete-incomplete -fexceptions -frtti -I/mnt/Main/LIBS/llvm-debug/include/")

        # SET(CMAKE_CXX_FLAGS "-g -Os -std=c++20 -fuse-ld=lld --analyze -flto -Wall -Wdelete-incomplete -fexceptions -frtti -I/mnt/Main/LIBS/llvm-debug/include/ -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS")
    endif()
else()
    if(IS_RELEASE)
        SET(CMAKE_CXX_FLAGS "-static -Oz -std=c++20 -fuse-ld=lld -flto -w -s -Wdelete-incomplete -fexceptions -frtti -IC:\\msys64\\mingw64\\include")
    else()
        SET(CMAKE_CXX_FLAGS "-static -g -Oz -std=c++20 -fuse-ld=lld -Wall -flto -Wdelete-incomplete -fexceptions -frtti -IC:\\msys64\\mingw64\\include")

        # SET(CMAKE_CXX_FLAGS "-g -Oz -std=c++20 -fuse-ld=lld --analyze -flto -Wall -Wdelete-incomplete -fexceptions -frtti -IC:\\msys64\\mingw64\\include\\c++\\v1 -IC:\\msys64\\mingw64\\include -D_HAS_STATIC_RTTI=0 -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -D_GLIBCXX_USE_CXX11_ABI=0")
    endif()
endif()

# LLVM Configuration to programmatically link required libs
#
#
if (IS_UNIX)
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
add_executable(${PROJECT_NAME}
    qat.cpp
    qat_sitter.cpp)
add_subdirectory(ast/)
add_subdirectory(lexer/)
add_subdirectory(parser/)
add_subdirectory(utils/)
add_subdirectory(IR/)
add_subdirectory(cli/)
add_subdirectory(backend/)
target_link_directories(${PROJECT_NAME}
    PRIVATE ast/
    PRIVATE lexer/
    PRIVATE parser/
    PRIVATE utils/
    PRIVATE cli/
    PRIVATE IR/
    PRIVATE backend/
    PRIVATE /usr/local/lib/)

if(IS_UNIX)
    if(IS_RELEASE)
        target_link_directories(${PROJECT_NAME} PRIVATE /mnt/Main/LIBS/llvm-release/lib)
    else()
        target_link_directories(${PROJECT_NAME} PRIVATE /mnt/Main/LIBS/llvm-debug/lib)
    endif()
else()
    target_link_directories(${PROJECT_NAME} PRIVATE C:\\msys64\\mingw64\\lib)
    # SET(FFI_LIBRARIES /c/msys64/mingw64/lib/libffi.a)
    # SET(ZLIB_LIBRARY /c/msys64/mingw64/lib/libz.a)
    # SET(LIBXML2_LIBRARY /c/msys64/mingw64/lib/libxml2.a)
endif()

if(IS_UNIX)
    target_link_libraries(${PROJECT_NAME}
        ${LLVM_LIBRARIES}
        LLVMBitWriter
        LLVMLinker
        stdc++
        stdc++fs
        rt
        dl
        pthread
        m
        z
        tinfo
        xml2
        QAT_AST
        QAT_LEXER
        QAT_PARSER
        QAT_IR
        QAT_UTILS
        QAT_CLI
        QAT_BACKEND)
else()
    target_link_libraries(${PROJECT_NAME}
        ${LLVM_LIBRARIES}
        LLVMBitWriter
        stdc++
        pthread
        m
        z
        xml2
        QAT_AST
        QAT_LEXER
        QAT_PARSER
        QAT_IR
        QAT_UTILS
        QAT_CLI
        QAT_BACKEND)
endif()

if(IS_UNIX)
    execute_process(COMMAND "export PATH=${CMAKE_BINARY_DIR}:$PATH")
endif()