cmake_minimum_required(VERSION 3.16.3)

PROJECT(qat
	VERSION "0.8.0"
	DESCRIPTION "Qat Programming Language"
	HOMEPAGE_URL "https://qat.dev")

set(LLVM_ROOT_DIR ${LLVM_DIR})

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

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

set(CMAKE_CXX_STANDARD 20)

if(IS_LINUX)
	find_package(LLVM REQUIRED CONFIG PATHS ${LLVM_ROOT_DIR}/lib/cmake/llvm NO_DEFAULT_PATH)
	find_package(Clang REQUIRED CONFIG PATHS ${LLVM_ROOT_DIR}/lib/cmake/clang NO_DEFAULT_PATH)
elseif(IS_WINDOWS)
	find_package(LLVM REQUIRED CONFIG PATHS ${LLVM_ROOT_DIR} NO_DEFAULT_PATH)
	find_package(Clang REQUIRED CONFIG PATHS ${LLVM_ROOT_DIR} NO_DEFAULT_PATH)
endif()

message(STATUS "Found LLVM version ${LLVM_VERSION}")
message(STATUS "Using LLVM-Config.cmake from ${LLVM_DIR}")

if(IS_LINUX)
	if(IS_RELEASE)
		set(CMAKE_CXX_FLAGS "-O2 -std=c++20 -fuse-ld=lld -flto -w -s -I${LLVM_ROOT_DIR}/include -fno-exceptions -funwind-tables -fno-rtti -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS")
	else()
		set(CMAKE_CXX_FLAGS "-g -O0 -std=c++20 -fuse-ld=lld -Wall -Wunused -I${LLVM_ROOT_DIR}/include -fno-exceptions -funwind-tables -fno-rtti -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS")
	endif()
elseif(IS_WINDOWS)
	if(IS_RELEASE)
		set(CMAKE_CXX_FLAGS "/DEBUG:NONE /SUBSYSTEM:CONSOLE /STATIC /OPT:REF /w /EHa /GR /EHsc")
	else()
		set(CMAKE_CXX_FLAGS "/DEBUG:FULL /SUBSYSTEM:CONSOLE /STATIC /OPT:REF /w /EHa /GR /EHsc")
	endif()
elseif(IS_MAC)
	if(IS_RELEASE)
		set(CMAKE_CXX_FLAGS "-O0 -std=c++20 -fuse-ld=lld -w -s")
	else()
		set(CMAKE_CXX_FLAGS "-g -O0 -std=c++20 -fuse-ld=lld -Wall")
	endif()
endif()

if(IS_WINDOWS)
	execute_process(COMMAND ${LLVM_ROOT_DIR}/bin/llvm-config.exe --link-static --libs
		OUTPUT_VARIABLE LLVM_LIBRARIES)
elseif(IS_MAC OR IS_LINUX)
	execute_process(COMMAND ${LLVM_ROOT_DIR}/bin/llvm-config --link-static --libs
		OUTPUT_VARIABLE LLVM_LIBRARIES)
endif()

string(STRIP "${LLVM_LIBRARIES}" LLVM_LIBRARIES)

add_executable(${PROJECT_NAME} qat.cc sitter.cc)
add_executable(qvm qvm.cc)
add_subdirectory(cli/)
add_subdirectory(utils/)
add_subdirectory(ast/)
add_subdirectory(lexer/)
add_subdirectory(parser/)
add_subdirectory(IR/)
add_subdirectory(vm/)

target_include_directories(${PROJECT_NAME} PUBLIC "${LLVM_ROOT_DIR}/include")
target_link_directories(${PROJECT_NAME}
	PUBLIC ast/ lexer/ parser/ utils/ cli/ IR/ vm/
	PRIVATE ${LLVM_ROOT_DIR}/lib ${LINK_DIRS} "${CMAKE_SOURCE_DIR}/../${SHARED_LIBPATH}")

target_link_directories(qvm
	PUBLIC vm/ utils/)

set(LLD_LIBRARIES lldCOFF lldCommon lldELF lldMachO lldMinGW lldWasm)

if(IS_LINUX)
	message(STATUS "Linking libraries ${LLVM_LIBRARIES}")
	target_link_libraries(
		${PROJECT_NAME} ${LLVM_LIBRARIES} ${LLD_LIBRARIES} clangBasic stdc++ rt pthread m z tinfo xml2 icuuc icudata lzma
		QAT_AST QAT_LEXER QAT_PARSER QAT_IR QAT_UTILS QAT_CLI QAT_VM
	)
elseif(IS_MAC)
	set(LLVM_LIB_PATHS "")
	string(REPLACE " " ";" LLVM_LIBRARIES "${LLVM_LIBRARIES}")

	foreach(llvmLib ${LLVM_LIBRARIES})
		string(REPLACE "-l" "" libName "${llvmLib}")
		list(APPEND LLVM_LIB_PATHS "${LLVM_ROOT_DIR}/lib/lib${libName}.a")
	endforeach()

	set(LLVM_LIB_PATHS ${LLVM_LIB_PATHS})
	message("LLVM Libraries are: ${LLVM_LIB_PATHS}")
	target_link_libraries(${PROJECT_NAME}
		${LLVM_LIB_PATHS} ${LLD_LIBRARIES} clangBasic stdc++ pthread
		"${LINK_DIRS}/libz.a" "${LINK_DIRS}/libzstd.a" "${LINK_DIRS}/libopenlibm.a"
		"${LINK_DIRS}/libcurses.a" "${LINK_DIRS}/libxml2.a" "${LINK_DIRS}/libiconv.a"
		"${LINK_DIRS}/libicui18n.a" "${LINK_DIRS}/libicuio.a" "${LINK_DIRS}/libicuuc.a"
		"${LINK_DIRS}/libicudata.a" "${LINK_DIRS}/libicutu.a" "${LZMA_DIR}/liblzma.a"
		QAT_AST QAT_LEXER QAT_PARSER QAT_IR QAT_UTILS QAT_CLI QAT_VM
	)
elseif(IS_WINDOWS)
	string(REPLACE " " ";" LLVM_LIBRARIES ${LLVM_LIBRARIES})
	message(STATUS "Linking libraries ${LLVM_LIBRARIES}")
	target_link_libraries(${PROJECT_NAME}
		QAT_UTILS QAT_CLI QAT_AST QAT_LEXER QAT_PARSER QAT_IR QAT_VM
		${LLVM_LIBRARIES} ${LLD_LIBRARIES} clangBasic
	)
endif()

set_target_properties(qvm PROPERTIES COMPILE_FLAGS "-std=c++20")
target_link_libraries(qvm stdc++ pthread QAT_VM QAT_UTILS)

message(STATUS "CMake install prefix is: ${CMAKE_INSTALL_PREFIX}")

if(IS_LINUX)
	include(GNUInstallDirs)
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}"
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
install(TARGETS qvm DESTINATION "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}"
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

install(DIRECTORY "${CMAKE_SOURCE_DIR}/std/" DESTINATION "${CMAKE_INSTALL_PREFIX}/std")