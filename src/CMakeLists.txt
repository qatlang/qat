cmake_minimum_required(VERSION 3.16.3)
PROJECT(qat
	VERSION "0.8.0"
	DESCRIPTION "Qat Programming Language"
	HOMEPAGE_URL "https://qat.dev")

if(NOT CMAKE_INSTALL_PREFIX STREQUAL "")
	message(STATUS "CMake installation prefix is: ${CMAKE_INSTALL_PREFIX}")
endif()

set(LLVM_ROOT_DIR ${LLVM_DIR})

set(is_mode_release OFF)

if(CMAKE_BUILD_TYPE STREQUAL Release)
	set(is_mode_release ON)
endif()

add_compile_definitions(is_mode_release=${is_mode_release})

set(is_os_linux OFF)
set(is_os_windows OFF)
set(is_os_macos OFF)
set(is_os_freebsd OFF)
set(is_os_haiku OFF)

set(is_arch_x64 OFF)
set(is_arch_x32 OFF)
set(is_arch_arm64 OFF)
set(is_arch_riscv64 OFF)

if((TARGET_ARCHITECTURE MATCHES "x86|i686") OR(CMAKE_SYSTEM_PROCESSOR MATCHES "X86|x86"))
	set(is_arch_x32 ON)
elseif((TARGET_ARCHITECTURE MATCHES "x64|x86_64|amd64") OR(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64"))
	set(is_arch_x64 ON)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "AARCH64|ARM64|arm64|aarch64")
	set(is_arch_arm64 ON)
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL riscv64)
	set(is_arch_riscv64 ON)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
	set(is_os_linux ON)
elseif(CMAKE_SYSTEM_NAME STREQUAL Windows)
	set(is_os_windows ON)
elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
	set(is_os_macos ON)
elseif(CMAKE_SYSTEM_NAME STREQUAL FreeBSD)
	set(is_os_freebsd ON)
elseif(CMAKE_SYSTEM_NAME STREQUAL Haiku)
	set(is_os_haiku ON)
endif()

add_compile_definitions(
	OS_IS_LINUX=$<IF:$<BOOL:${is_os_linux}>,1,0>
	OS_IS_WINDOWS=$<IF:$<BOOL:${is_os_windows}>,1,0>
	OS_IS_MAC=$<IF:$<BOOL:${is_os_macos}>,1,0>
	OS_IS_FREEBSD=$<IF:$<BOOL:${is_os_freebsd}>,1,0>
	OS_IS_HAIKU=$<IF:$<BOOL:${is_os_haiku}>,1,0>
)

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

if(is_mode_release)
	add_compile_definitions(
		QAT_BUILD_TYPE=Release
		NDEBUG=1
	)
else()
	add_compile_definitions(
		QAT_BUILD_TYPE=Debug
		NDEBUG=0
	)
endif()

set(CMAKE_CXX_STANDARD 20)

find_package(LLVM REQUIRED CONFIG PATHS "${LLVM_ROOT_DIR}/lib/cmake/llvm" NO_DEFAULT_PATH)
find_package(Clang REQUIRED CONFIG PATHS "${LLVM_ROOT_DIR}/lib/cmake/clang" NO_DEFAULT_PATH)

message(STATUS "Found LLVM version ${LLVM_VERSION}")
message(STATUS "Using LLVM-Config.cmake from ${LLVM_DIR}")

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost REQUIRED COMPONENTS filesystem PATHS "${BOOST_DIR}/lib/cmake" NO_DEFAULT_PATH)

if(is_os_linux)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -ftime-trace -fexceptions -fno-rtti ${LLVM_DEFINITIONS}")

	if(NOT CMAKE_CROSSCOMPILING)
		set(CMAKE_CXX_FLAGS "-fuse-ld=lld ${CMAKE_CXX_FLAGS}")
	endif()

	if(is_mode_release)
		set(CMAKE_CXX_FLAGS "-O3 -w -s -flto ${CMAKE_CXX_FLAGS}")
	else()
		set(CMAKE_CXX_FLAGS "-g -O0 -Wall -Wunused ${CMAKE_CXX_FLAGS}")
	endif()
elseif(is_os_macos)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=c++20 -flto -w -s -funwind-tables -fno-rtti")
	if (CMAKE_CROSSCOMPILING)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fuse-ld=lld -stdlib=libc++")
	endif()
elseif(is_os_windows)
	if(MINGW)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=c++20 -w -fpermissive ${LLVM_DEFINITIONS}")
	else()
		if(is_mode_release)
			set(CMAKE_CXX_FLAGS "/DEBUG:NONE /std:c++20 /O2 /W0 /EHa /GR /D _WIN32_WINDOWS=0x0A00")
			set(CMAKE_EXE_LINKER_FLAGS "/OPT:REF /SUBSYSTEM:CONSOLE")
		else()
			set(CMAKE_CXX_FLAGS "/DEBUG:FULL /std:c++20 /W0 /EHa /GR /D _WIN32_WINDOWS=0x0A00")
		endif()
	endif()
endif()

set(LLVM_LIBRARIES
	-lLLVMWindowsManifest -lLLVMXRay -lLLVMLibDriver -lLLVMDlltoolDriver -lLLVMTelemetry -lLLVMTextAPIBinaryReader -lLLVMCoverage -lLLVMLineEditor -lLLVMXCoreDisassembler -lLLVMXCoreCodeGen -lLLVMXCoreDesc -lLLVMXCoreInfo -lLLVMX86TargetMCA -lLLVMX86Disassembler -lLLVMX86AsmParser -lLLVMX86CodeGen -lLLVMX86Desc -lLLVMX86Info -lLLVMWebAssemblyDisassembler -lLLVMWebAssemblyAsmParser -lLLVMWebAssemblyCodeGen -lLLVMWebAssemblyUtils -lLLVMWebAssemblyDesc -lLLVMWebAssemblyInfo -lLLVMVEDisassembler -lLLVMVEAsmParser -lLLVMVECodeGen -lLLVMVEDesc -lLLVMVEInfo -lLLVMSystemZDisassembler -lLLVMSystemZAsmParser -lLLVMSystemZCodeGen -lLLVMSystemZDesc -lLLVMSystemZInfo -lLLVMSPIRVCodeGen -lLLVMSPIRVDesc -lLLVMSPIRVInfo -lLLVMSPIRVAnalysis -lLLVMSparcDisassembler -lLLVMSparcAsmParser -lLLVMSparcCodeGen -lLLVMSparcDesc -lLLVMSparcInfo -lLLVMRISCVTargetMCA -lLLVMRISCVDisassembler -lLLVMRISCVAsmParser -lLLVMRISCVCodeGen -lLLVMRISCVDesc -lLLVMRISCVInfo -lLLVMPowerPCDisassembler -lLLVMPowerPCAsmParser -lLLVMPowerPCCodeGen -lLLVMPowerPCDesc -lLLVMPowerPCInfo -lLLVMNVPTXCodeGen -lLLVMNVPTXDesc -lLLVMNVPTXInfo -lLLVMMSP430Disassembler -lLLVMMSP430AsmParser -lLLVMMSP430CodeGen -lLLVMMSP430Desc -lLLVMMSP430Info -lLLVMMipsDisassembler -lLLVMMipsAsmParser -lLLVMMipsCodeGen -lLLVMMipsDesc -lLLVMMipsInfo -lLLVMLoongArchDisassembler -lLLVMLoongArchAsmParser -lLLVMLoongArchCodeGen -lLLVMLoongArchDesc -lLLVMLoongArchInfo -lLLVMLanaiDisassembler -lLLVMLanaiCodeGen -lLLVMLanaiAsmParser -lLLVMLanaiDesc -lLLVMLanaiInfo -lLLVMHexagonDisassembler -lLLVMHexagonCodeGen -lLLVMHexagonAsmParser -lLLVMHexagonDesc -lLLVMHexagonInfo -lLLVMBPFDisassembler -lLLVMBPFAsmParser -lLLVMBPFCodeGen -lLLVMBPFDesc -lLLVMBPFInfo -lLLVMAVRDisassembler -lLLVMAVRAsmParser -lLLVMAVRCodeGen -lLLVMAVRDesc -lLLVMAVRInfo -lLLVMARMDisassembler -lLLVMARMAsmParser -lLLVMARMCodeGen -lLLVMARMDesc -lLLVMARMUtils -lLLVMARMInfo -lLLVMAMDGPUTargetMCA -lLLVMAMDGPUDisassembler -lLLVMAMDGPUAsmParser -lLLVMAMDGPUCodeGen -lLLVMAMDGPUDesc -lLLVMAMDGPUUtils -lLLVMAMDGPUInfo -lLLVMAArch64Disassembler -lLLVMAArch64AsmParser -lLLVMAArch64CodeGen -lLLVMAArch64Desc -lLLVMAArch64Utils -lLLVMAArch64Info -lLLVMOrcDebugging -lLLVMOrcJIT -lLLVMWindowsDriver -lLLVMMCJIT -lLLVMJITLink -lLLVMInterpreter -lLLVMExecutionEngine -lLLVMRuntimeDyld -lLLVMOrcTargetProcess -lLLVMOrcShared -lLLVMDWP -lLLVMDebugInfoLogicalView -lLLVMDebugInfoGSYM -lLLVMOption -lLLVMObjectYAML -lLLVMObjCopy -lLLVMMCA -lLLVMMCDisassembler -lLLVMLTO -lLLVMPasses -lLLVMHipStdPar -lLLVMCFGuard -lLLVMCoroutines -lLLVMipo -lLLVMVectorize -lLLVMSandboxIR -lLLVMLinker -lLLVMInstrumentation -lLLVMFrontendOpenMP -lLLVMFrontendOffloading -lLLVMFrontendOpenACC -lLLVMFrontendHLSL -lLLVMFrontendDriver -lLLVMFrontendAtomic -lLLVMExtensions -lLLVMDWARFLinkerParallel -lLLVMDWARFLinkerClassic -lLLVMDWARFLinker -lLLVMGlobalISel -lLLVMMIRParser -lLLVMAsmPrinter -lLLVMSelectionDAG -lLLVMCodeGen -lLLVMTarget -lLLVMObjCARCOpts -lLLVMCodeGenTypes -lLLVMCGData -lLLVMIRPrinter -lLLVMInterfaceStub -lLLVMFileCheck -lLLVMFuzzMutate -lLLVMScalarOpts -lLLVMInstCombine -lLLVMAggressiveInstCombine -lLLVMTransformUtils -lLLVMBitWriter -lLLVMAnalysis -lLLVMProfileData -lLLVMSymbolize -lLLVMDebugInfoBTF -lLLVMDebugInfoPDB -lLLVMDebugInfoMSF -lLLVMDebugInfoCodeView -lLLVMDebugInfoDWARF -lLLVMObject -lLLVMTextAPI -lLLVMMCParser -lLLVMIRReader -lLLVMAsmParser -lLLVMMC -lLLVMBitReader -lLLVMFuzzerCLI -lLLVMCore -lLLVMRemarks -lLLVMBitstreamReader -lLLVMBinaryFormat -lLLVMTargetParser -lLLVMTableGen -lLLVMSupport -lLLVMDemangle -lclangBasic -llldCOFF -llldCommon -llldELF -llldMachO -llldMinGW -llldWasm)

add_executable(${PROJECT_NAME} qat.cc sitter.cc)
add_subdirectory(cli/)
add_subdirectory(utils/)
add_subdirectory(ast/)
add_subdirectory(lexer/)
add_subdirectory(parser/)
add_subdirectory(IR/)

target_include_directories(${PROJECT_NAME} PUBLIC "${LLVM_INCLUDE_DIRS}" "${Boost_INCLUDE_DIRS}" "${LIBS_DIR}/icu4c/include")

target_link_directories(${PROJECT_NAME}
	PUBLIC ast/ lexer/ parser/ utils/ cli/ IR/
	PRIVATE ${LLVM_ROOT_DIR}/lib ${LINK_DIRS}
)

if(is_os_macos AND CMAKE_CROSSCOMPILING)
	target_include_directories(${PROJECT_NAME} ${CMAKE_SYSROOT}/usr/include)
	target_link_directories(${PROJECT_NAME} ${CMAKE_SYSROOT}/usr/lib)
endif()

set(LLVM_LIB_PATHS "")

foreach(llvmLib ${LLVM_LIBRARIES})
	string(REPLACE "-l" "" libName "${llvmLib}")

	if(EXISTS "${LLVM_ROOT_DIR}/lib/lib${libName}.a")
		list(APPEND LLVM_LIB_PATHS "${LLVM_ROOT_DIR}/lib/lib${libName}.a")
	endif()
endforeach()

set(LLVM_LIB_PATHS ${LLVM_LIB_PATHS})

if(is_os_linux)
	target_link_libraries(
		${PROJECT_NAME} ${LLVM_LIB_PATHS} QAT_AST QAT_LEXER QAT_PARSER QAT_IR QAT_UTILS QAT_CLI
		stdc++ rt pthread m
		"${LIBS_DIR}/icu4c/lib/libicuuc.a" "${LIBS_DIR}/icu4c/lib/libicudata.a"
	)
elseif(is_os_macos)
	target_link_libraries(${PROJECT_NAME}
		${LLVM_LIB_PATHS} QAT_AST QAT_LEXER QAT_PARSER QAT_IR QAT_UTILS QAT_CLI
		"${LIBS_DIR}/icu4c/lib/libicuuc.a" "${LIBS_DIR}/icu4c/lib/libicudata.a"
	)
	if (CMAKE_CROSSCOMPILING)
		target_link_libraries(${PROJECT_NAME}
		"${CMAKE_SYSROOT}/usr/lib/libc++.tbd")
	endif()
elseif(is_os_windows)
	string(REPLACE " " ";" LLVM_LIBRARIES "${LLVM_LIBRARIES}")
	string(REPLACE "-l" "" LLVM_LIBRARIES "${LLVM_LIBRARIES}")
	message(STATUS "Linking libraries ${LLVM_LIBRARIES}")

	if(MINGW)
		target_link_libraries(${PROJECT_NAME}
			QAT_UTILS QAT_CLI QAT_AST QAT_LEXER QAT_PARSER QAT_IR
			${LLVM_LIBRARIES} pthread
			"${LIBS_DIR}/icu4c/lib/libicuuc.a" "${LIBS_DIR}/icu4c/lib/libicudata.a"
		)
	else()
		target_link_libraries(${PROJECT_NAME}
			QAT_UTILS QAT_CLI QAT_AST QAT_LEXER QAT_PARSER QAT_IR
			${LLVM_LIBRARIES} ${LLD_LIBRARIES}
			"${LIBS_DIR}/icu4c/lib/icuuc.lib" "${LIBS_DIR}/icu4c/lib/icudt.lib"
		)
	endif()
endif()

if(is_os_linux AND(NOT CMAKE_CROSSCOMPILING))
	include(GNUInstallDirs)
endif()
if(is_os_windows)
	set(BIN_EXT ".exe")
endif()

install(TARGETS "${PROJECT_NAME}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

string(REPLACE "." ";" LLVM_VERSION_LIST ${LLVM_VERSION})
list(GET LLVM_VERSION_LIST 0 CLANG_VERSION)

set(CLANG_PATH "${LLVM_ROOT_DIR}/bin/clang-${CLANG_VERSION}${BIN_EXT}")

if(NOT EXISTS CLANG_PATH)
	set(CLANG_PATH "${LLVM_ROOT_DIR}/bin/clang${BIN_EXT}")
endif()

install(FILES "${CLANG_PATH}" TYPE "BIN" RENAME "qat-clang${BIN_EXT}"
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

if(is_os_windows AND(NOT MINGW))
	file(GLOB ICU4C_LIBS "${LIBS_DIR}/icu4c/bin/icudt*.dll" "${LIBS_DIR}/icu4c/bin/icuuc*.dll")

	foreach(icu_file ${ICU4C_LIBS})
		install(FILES "${icu_file}" TYPE "BIN")
	endforeach()
endif()

install(FILES "${LLVM_ROOT_DIR}/bin/llvm-ar${BIN_EXT}" TYPE "BIN" RENAME "qat-llvm-ar${BIN_EXT}"
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
install(FILES "${LLVM_ROOT_DIR}/bin/lld${BIN_EXT}" TYPE "BIN"
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/std/" DESTINATION "std")
install(DIRECTORY DESTINATION "toolchain")

# CPack for bundling the release files
if(is_os_windows)
	set(CPACK_GENERATOR "ZIP")
else()
	set(CPACK_GENERATOR "TGZ")
endif()

set(CPACK_VERBATIM_VARIABLES YES)

if(is_os_macos)
	set(SYSTEM_NAME_LOWERCASE "macos")
else()
	string(TOLOWER "${CMAKE_SYSTEM_NAME}" SYSTEM_NAME_LOWERCASE)
endif()

if(TARGET_ARCHITECTURE STREQUAL "")
	set(TARGET_ARCHITECTURE "${CMAKE_SYSTEM_PROCESSOR}")
endif()

string(TOLOWER "${TARGET_ARCHITECTURE}" SYSTEM_PROCESSOR_LOWERCASE)
set(BUNDLE_FILE_NAME "qat-${PROJECT_VERSION}-${SYSTEM_NAME_LOWERCASE}-${SYSTEM_PROCESSOR_LOWERCASE}")
message(STATUS "Bundle file name is ${BUNDLE_FILE_NAME}")
set(CPACK_PACKAGE_FILE_NAME "${BUNDLE_FILE_NAME}")
set(CPACK_ARCHIVE_THREADS 0)
include(CPack)
