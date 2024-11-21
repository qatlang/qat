cmake_minimum_required(VERSION 3.16.3)

PROJECT(qat
	VERSION "0.9.0"
	DESCRIPTION "Qat Programming Language"
	HOMEPAGE_URL "https://qat.dev")

set(LLVM_ROOT_DIR ${LLVM_DIR})

set(is_mode_release OFF)
if (CMAKE_BUILD_TYPE STREQUAL Release)
  set(is_mode_release ON)
endif()
add_compile_definitions(is_mode_release=${is_mode_release})

set(is_os_linux   OFF)
set(is_os_windows OFF)
set(is_os_macos   OFF)
set(is_os_freebsd OFF)
set(is_os_haiku   OFF)

set(is_arch_x64     OFF)
set(is_arch_arm64   OFF)
set(is_arch_riscv64 OFF)

if (CMAKE_SYSTEM_PROCESSOR STREQUAL x86_64)
  set(is_arch_x64 ON)
elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL arm64)
  set(is_arch_arm64 ON)
elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL aarch64)
  set(is_arch_arm64 ON)
elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL riscv64)
  set(is_arch_riscv64 ON)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL Linux)
  set(is_os_linux ON)
elseif (CMAKE_SYSTEM_NAME STREQUAL Windows)
  set(is_os_windows ON)
  if (RUNTIME STREQUAL mingw)
	 set(is_runtime_mingw ON)
	 set(is_runtime_microsoft OFF)
  else()
	 set(is_runtime_mingw OFF)
	 set(is_runtime_microsoft ON)
  endif()
elseif (CMAKE_SYSTEM_NAME STREQUAL Darwin)
  set(is_os_macos ON)
elseif (CMAKE_SYSTEM_NAME STREQUAL FreeBSD)
  set(is_os_freebsd ON)
elseif (CMAKE_SYSTEM_NAME STREQUAL Haiku)
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

if (is_runtime_mingw)
  add_compile_definitions(_M_X64=true)
endif()

if(is_mode_release)
  add_compile_definitions(
	 QAT_BUILD_TYPE=Release
	 NDEBUG=1)
else()
  add_compile_definitions(
	 QAT_BUILD_TYPE=Debug
	 NDEBUG=0)
endif()

set(CMAKE_CXX_STANDARD 20)

if(is_os_windows)
	find_package(LLVM REQUIRED CONFIG PATHS ${LLVM_ROOT_DIR} NO_DEFAULT_PATH)
	find_package(Clang REQUIRED CONFIG PATHS ${LLVM_ROOT_DIR} NO_DEFAULT_PATH)
	if (is_runtime_mingw)
	  set(LIBLZMA_HAS_AUTO_DECODER 1)
	  set(LIBLZMA_HAS_EASY_DECODER 0)
	  set(LIBLZMA_HAS_LZMA_PRESET 1)
	  set(LIBLZMA_LIBRARY "${LIBS_DIR}/lzma/lib/liblzma.a")
	  set(LIBLZMA_INCLUDE_DIR "${LIBS_DIR}/lzma/include")
	  set(LIBLZMA_VERSION "5.6.3")
	  add_library(LibLZMA STATIC IMPORTED)
	  set_target_properties(LibLZMA PROPERTIES
	    IMPORTED_LOCATION "${LIBS_DIR}/lzma/lib/liblzma.a"
	    INTERFACE_INCLUDE_DIRECTORIES "${LIBS_DIR}/lzma/include")
	endif()
 else()
	if (is_arch_arm64 OR is_arch_riscv64)
	  add_library(ZLIB::ZLIB STATIC IMPORTED)
	  set_target_properties(ZLIB::ZLIB PROPERTIES
	    IMPORTED_LOCATION "${LIBS_DIR}/zlib/lib/libz.a"
	    INTERFACE_INCLUDE_DIRECTORIES "${LIBS_DIR}/zlib/include")
	  set(ZLIB_LIBRARY "${LIBS_DIR}/zlib/lib/libz.a")
	  set(ZLIB_INCLUDE_DIRS "${LIBS_DIR}/zlib/include")

	  add_library(zstd STATIC IMPORTED)
	  set_target_properties(zstd PROPERTIES
	    IMPORTED_LOCATION "${LIBS_DIR}/zstd/lib/libzstd.a"
	    INTERFACE_INCLUDE_DIRECTORIES "${LIBS_DIR}/zstd/include")

	  add_library(libxml2 STATIC IMPORTED)
	  set_target_properties(libxml2 PROPERTIES
	    IMPORTED_LOCATION "${LIBS_DIR}/libxml2/lib/libxml2.a"
	    INTERFACE_INCLUDE_DIRECTORIES "${LIBS_DIR}/libxml2/include")

	  if (is_arch_riscv64)
		 # Change this to apply to other architectures as well?
		 add_library(LibEdit STATIC IMPORTED)
		 set_target_properties(LibEdit PROPERTIES
		   IMPORTED_LOCATION "${LIBS_DIR}/libedit/lib/libedit.a"
		   INTERFACE_INCLUDE_DIRECTORIES "${LIBS_DIR}/libedit/include")
		 set(LibEdit_LIBRARIES "${LIBS_DIR}/libedit/lib/libedit.a")
		 set(LibEdit_INCLUDE_DIRS "${LIBS_DIR}/libedit/include")
	  endif()

	  set(TERMINFO_PATH "${LIBS_DIR}/ncurses/lib/libtinfo.a")
	  set(NCURSES_INCLUDE "${LIBS_DIR}/ncurses/include")

	  if (NOT EXISTS ${TERMINFO_PATH})
		 if (EXISTS "${LIBS_DIR}/ncurses/lib/libncursesw.a")
			set(TERMINFO_PATH "${LIBS_DIR}/ncurses/lib/libncursesw.a")
		 elseif (EXISTS "${LIBS_DIR}/ncurses/lib/libncurses.a")
			set(TERMINFO_PATH "${LIBS_DIR}/ncurses/lib/libncurses.a")
		 elseif(EXISTS "${LIBS_DIR}/ncurses/lib/libcurses.a")
			set(TERMINFO_PATH "${LIBS_DIR}/ncurses/lib/libcurses.a")
		 endif()
	  endif()

	  add_library(Terminfo::terminfo STATIC IMPORTED)
 	  set_target_properties(Terminfo::terminfo PROPERTIES
		 IMPORTED_LOCATION ${TERMINFO_PATH}
		 INTERFACE_INCLUDE_DIRECTORIES ${NCURSES_INCLUDE})
	  set(Terminfo_LIBRARIES ${TERMINFO_PATH})
	  set(Terminfo_LINKABLE ${TERMINFO_PATH})
	  set(Terminfo_INCLUDE_DIRS ${NCURSES_INCLUDE})

	  set(Iconv_LIBRARY "${LIBS_DIR}/libiconv/lib/libiconv.a")

	  set(LIBLZMA_HAS_AUTO_DECODER 1)
	  set(LIBLZMA_HAS_EASY_DECODER 0)
	  set(LIBLZMA_HAS_LZMA_PRESET 1)
	  set(LIBLZMA_LIBRARY "${LIBS_DIR}/lzma/lib/liblzma.a")
	  set(LIBLZMA_INCLUDE_DIR "${LIBS_DIR}/lzma/include")
	  set(LIBLZMA_VERSION "5.6.3")
	  add_library(LibLZMA STATIC IMPORTED)
	  set_target_properties(LibLZMA PROPERTIES
	    IMPORTED_LOCATION "${LIBS_DIR}/lzma/lib/liblzma.a"
	    INTERFACE_INCLUDE_DIRECTORIES "${LIBS_DIR}/lzma/include")
	endif()
	find_package(LLVM REQUIRED CONFIG PATHS ${LLVM_ROOT_DIR}/lib/cmake/llvm NO_DEFAULT_PATH)
	find_package(Clang REQUIRED CONFIG PATHS ${LLVM_ROOT_DIR}/lib/cmake/clang NO_DEFAULT_PATH)
endif()

message(STATUS "Found LLVM version ${LLVM_VERSION}")
message(STATUS "Using LLVM-Config.cmake from ${LLVM_DIR}")

if(is_os_linux)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -I${LLVM_ROOT_DIR}/include -fexceptions -fno-rtti -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS")
   if (NOT CMAKE_CROSSCOMPILING)
	  set(CMAKE_CXX_FLAGS "-fuse-ld=lld ${CMAKE_CXX_FLAGS}")
   endif()

	if(is_mode_release)
	  if (is_arch_x64)
		 set(CMAKE_CXX_FLAGS "-O3 -w -s -flto ${CMAKE_CXX_FLAGS}")
	  elseif(is_arch_arm64 OR is_arch_riscv64)
		 set(CMAKE_CXX_FLAGS "-O3 -w -Wl,--no-undefined ${CMAKE_CXX_FLAGS}")
	  endif()
	else()
		set(CMAKE_CXX_FLAGS "-g -O0 -Wall -Wunused ${CMAKE_CXX_FLAGS}")
	endif()
elseif(is_os_macos)
	if (is_arch_x64)
	  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=c++20 -stdlib=libc++ -fuse-ld=lld -flto -w -s -L${CMAKE_SYSROOT}/usr/lib -funwind-tables -fno-rtti")
   elseif(is_arch_arm64)
	  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=c++20 -stdlib=libc++ -fuse-ld=lld -flto -w -s -L${CMAKE_SYSROOT}/usr/lib -funwind-tables -fno-rtti")
	endif()
elseif(is_os_windows)
	if (is_runtime_mingw)
	  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=c++20 -w -fpermissive -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS")
	else()
	  if(is_mode_release)
		 set(CMAKE_CXX_FLAGS "/DEBUG:NONE /STD:c++20 /SUBSYSTEM:CONSOLE /STATIC /OPT:REF /w /EHa /GR /EHsc")
	  else()
		 set(CMAKE_CXX_FLAGS "/DEBUG:FULL /STD:c++20 /SUBSYSTEM:CONSOLE /STATIC /OPT:REF /w /EHa /GR /EHsc")
	  endif()
   endif()
endif()

if (is_arch_arm64)
  set(BOOST_INCLUDE_DIR "${LIBS_DIR}/boost/include")
endif()

set(DEFAULT_LLVM_LIBS
  -lLLVMWindowsManifest -lLLVMWindowsDriver -lLLVMXRay -lLLVMLibDriver -lLLVMDlltoolDriver -lLLVMCoverage -lLLVMLineEditor -lLLVMM68kDisassembler
  -lLLVMM68kAsmParser -lLLVMM68kCodeGen -lLLVMM68kDesc -lLLVMM68kInfo -lLLVMXCoreDisassembler -lLLVMXCoreCodeGen -lLLVMXCoreDesc -lLLVMXCoreInfo
  -lLLVMX86TargetMCA -lLLVMX86Disassembler -lLLVMX86AsmParser -lLLVMX86CodeGen -lLLVMX86Desc -lLLVMX86Info -lLLVMWebAssemblyDisassembler
  -lLLVMWebAssemblyAsmParser -lLLVMWebAssemblyCodeGen -lLLVMWebAssemblyDesc -lLLVMWebAssemblyUtils -lLLVMWebAssemblyInfo -lLLVMVEDisassembler
  -lLLVMVEAsmParser -lLLVMVECodeGen -lLLVMVEDesc -lLLVMVEInfo -lLLVMSystemZDisassembler -lLLVMSystemZAsmParser -lLLVMSystemZCodeGen -lLLVMSystemZDesc
  -lLLVMSystemZInfo -lLLVMSparcDisassembler -lLLVMSparcAsmParser -lLLVMSparcCodeGen -lLLVMSparcDesc -lLLVMSparcInfo -lLLVMRISCVDisassembler
  -lLLVMRISCVAsmParser -lLLVMRISCVCodeGen -lLLVMRISCVDesc -lLLVMRISCVInfo -lLLVMPowerPCDisassembler -lLLVMPowerPCAsmParser -lLLVMPowerPCCodeGen
  -lLLVMPowerPCDesc -lLLVMPowerPCInfo -lLLVMNVPTXCodeGen -lLLVMNVPTXDesc -lLLVMNVPTXInfo -lLLVMMSP430Disassembler -lLLVMMSP430AsmParser
  -lLLVMMSP430CodeGen -lLLVMMSP430Desc -lLLVMMSP430Info -lLLVMMipsDisassembler -lLLVMMipsAsmParser -lLLVMMipsCodeGen -lLLVMMipsDesc -lLLVMMipsInfo
  -lLLVMLanaiDisassembler -lLLVMLanaiCodeGen -lLLVMLanaiAsmParser -lLLVMLanaiDesc -lLLVMLanaiInfo -lLLVMHexagonDisassembler -lLLVMHexagonCodeGen
  -lLLVMHexagonAsmParser -lLLVMHexagonDesc -lLLVMHexagonInfo -lLLVMBPFDisassembler -lLLVMBPFAsmParser -lLLVMBPFCodeGen -lLLVMBPFDesc -lLLVMBPFInfo
  -lLLVMAVRDisassembler -lLLVMAVRAsmParser -lLLVMAVRCodeGen -lLLVMAVRDesc -lLLVMAVRInfo -lLLVMARMDisassembler -lLLVMARMAsmParser -lLLVMARMCodeGen
  -lLLVMARMDesc -lLLVMARMUtils -lLLVMARMInfo -lLLVMAMDGPUTargetMCA -lLLVMAMDGPUDisassembler -lLLVMAMDGPUAsmParser -lLLVMAMDGPUCodeGen -lLLVMAMDGPUDesc
  -lLLVMAMDGPUUtils -lLLVMAMDGPUInfo -lLLVMAArch64Disassembler -lLLVMAArch64AsmParser -lLLVMAArch64CodeGen -lLLVMAArch64Desc -lLLVMAArch64Utils
  -lLLVMAArch64Info -lLLVMPerfJITEvents -lLLVMOrcJIT -lLLVMMCJIT -lLLVMJITLink -lLLVMInterpreter -lLLVMExecutionEngine -lLLVMRuntimeDyld
  -lLLVMOrcTargetProcess -lLLVMOrcShared -lLLVMDWP -lLLVMDebugInfoGSYM -lLLVMOption -lLLVMObjectYAML -lLLVMObjCopy -lLLVMMCA -lLLVMMCDisassembler
  -lLLVMLTO -lLLVMCFGuard -lLLVMFrontendOpenACC -lLLVMExtensions -lPolly -lPollyISL -lLLVMPasses -lLLVMObjCARCOpts -lLLVMCoroutines -lLLVMipo
  -lLLVMInstrumentation -lLLVMVectorize -lLLVMLinker -lLLVMFrontendOpenMP -lLLVMDWARFLinker -lLLVMGlobalISel -lLLVMMIRParser -lLLVMAsmPrinter
  -lLLVMSelectionDAG -lLLVMCodeGen -lLLVMIRReader -lLLVMAsmParser -lLLVMInterfaceStub -lLLVMFileCheck -lLLVMFuzzMutate -lLLVMTarget -lLLVMScalarOpts
  -lLLVMInstCombine -lLLVMAggressiveInstCombine -lLLVMTransformUtils -lLLVMBitWriter -lLLVMAnalysis -lLLVMProfileData -lLLVMSymbolize -lLLVMDebugInfoPDB
  -lLLVMDebugInfoMSF -lLLVMDebugInfoDWARF -lLLVMObject -lLLVMTextAPI -lLLVMMCParser -lLLVMMC -lLLVMDebugInfoCodeView -lLLVMDebugInfoLogicalView
  -lLLVMBitReader -lLLVMFuzzerCLI -lLLVMCore -lLLVMRemarks -lLLVMBitstreamReader -lLLVMBinaryFormat -lLLVMTableGen -lLLVMSupport -lLLVMDemangle
  -lLLVMIRPrinter -lLLVMCodeGenTypes -lLLVMTargetParser -lLLVMAsmParser -lLLVMAsmPrinter -lLLVMLoongArchAsmParser -lLLVMLoongArchCodeGen
  -lLLVMLoongArchDesc -lLLVMLoongArchDisassembler -lLLVMLoongArchInfo)

if(is_os_windows)
  if (is_runtime_mingw)
	 set(LLVM_LIBRARIES ${DEFAULT_LLVM_LIBS})
  else()
    execute_process(COMMAND ${LLVM_ROOT_DIR}/bin/llvm-config.exe --link-static --libs
		OUTPUT_VARIABLE LLVM_LIBRARIES)
  endif()
elseif(is_os_linux)
  if (CMAKE_CROSSCOMPILING)
	 set(LLVM_LIBRARIES ${DEFAULT_LLVM_LIBS})
  else()
	 execute_process(COMMAND ${LLVM_ROOT_DIR}/bin/llvm-config --link-static --libs
	   OUTPUT_VARIABLE LLVM_LIBRARIES)
  endif()
elseif(is_os_macos)
  set(LLVM_LIBRARIES ${DEFAULT_LLVM_LIBS})
endif()

string(STRIP "${LLVM_LIBRARIES}" LLVM_LIBRARIES)

add_executable(${PROJECT_NAME} qat.cc sitter.cc)
add_subdirectory(cli/)
add_subdirectory(utils/)
add_subdirectory(ast/)
add_subdirectory(lexer/)
add_subdirectory(parser/)
add_subdirectory(IR/)

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/../include" "${LLVM_ROOT_DIR}/include" "${BOOST_INCLUDE_DIR}")

if (is_os_macos)
  if (is_arch_x64)
	 target_link_directories(${PROJECT_NAME}
		PUBLIC ast/ lexer/ parser/ utils/ cli/ IR/
		PRIVATE ${LLVM_ROOT_DIR}/lib ${LINK_DIRS})
  else()
	 target_link_directories(${PROJECT_NAME}
		PUBLIC ast/ lexer/ parser/ utils/ cli/ IR/
		PRIVATE ${LLVM_ROOT_DIR}/lib ${LINK_DIRS} ${CMAKE_SYSROOT}/usr/lib)
  endif()
else()
  target_link_directories(${PROJECT_NAME}
	 PUBLIC ast/ lexer/ parser/ utils/ cli/ IR/
	 ${LLVM_ROOT_DIR}/lib ${LINK_DIRS})
endif()

set(LLD_LIBRARIES lldCOFF lldCommon lldELF lldMachO lldMinGW lldWasm)

if(is_os_linux)
  if (is_arch_x64)
	 target_link_libraries(
		${PROJECT_NAME} ${LLVM_LIBRARIES} ${LLD_LIBRARIES} clangBasic stdc++ rt pthread m z tinfo xml2 icuuc icudata lzma
		QAT_AST QAT_LEXER QAT_PARSER QAT_IR QAT_UTILS QAT_CLI)
  elseif(is_arch_arm64 OR is_arch_riscv64)
	 set(LLVM_LIB_PATHS "")
	 foreach(llvmLib ${LLVM_LIBRARIES})
		string(REPLACE "-l" "" libName "${llvmLib}")
		if (EXISTS "${LLVM_ROOT_DIR}/lib/lib${libName}.a")
		  list(APPEND LLVM_LIB_PATHS "${LLVM_ROOT_DIR}/lib/lib${libName}.a")
		endif()
	 endforeach()
	
	 set(LLD_LIB_PATHS "")
	 string(REPLACE " " ";" LLD_LIBRARIES "${LLD_LIBRARIES}")
	 foreach(lldLib ${LLD_LIBRARIES})
		string(REPLACE "-l" "" libName "${lldLib}")
		list(APPEND LLD_LIB_PATHS "${LLVM_ROOT_DIR}/lib/lib${libName}.a")
	 endforeach()
	 set(LLVM_LIB_PATHS ${LLVM_LIB_PATHS})

    if (CMAKE_CROSSCOMPILING)
		target_link_libraries(
		  ${PROJECT_NAME} QAT_AST QAT_LEXER QAT_PARSER QAT_IR QAT_UTILS QAT_CLI
		  ${LLVM_LIB_PATHS} ${LLD_LIB_PATHS} "${LLVM_ROOT_DIR}/lib/libclangBasic.a" stdc++ rt pthread m
		  "${LIBS_DIR}/libxml2/lib/libxml2.a" "${LIBS_DIR}/zlib/lib/libz.a"
		  "${LIBS_DIR}/icu4c/lib/libicuuc.a" "${LIBS_DIR}/icu4c/lib/libicudata.a"
		  "${LIBS_DIR}/lzma/lib/liblzma.a" "${LIBS_DIR}/ncurses/lib/libtinfo.a"
	   )
    else()
	   target_link_libraries(
		  ${PROJECT_NAME} ${LLVM_LIB_PATHS} ${LLD_LIB_PATHS} clangBasic stdc++ rt pthread m xml2
		  QAT_AST QAT_LEXER QAT_PARSER QAT_IR QAT_UTILS QAT_CLI
	   )
	 endif()
  endif()
elseif(is_os_macos)
   set(LLVM_LIB_PATHS "")

	foreach(llvmLib ${LLVM_LIBRARIES})
	  string(REPLACE "-l" "" libName "${llvmLib}")
	  if (EXISTS "${LLVM_ROOT_DIR}/lib/lib${libName}.a")
		 list(APPEND LLVM_LIB_PATHS "${LLVM_ROOT_DIR}/lib/lib${libName}.a")
	  endif()
	endforeach()
	
	set(LLD_LIB_PATHS "")
	string(REPLACE " " ";" LLD_LIBRARIES "${LLD_LIBRARIES}")

	foreach(lldLib ${LLD_LIBRARIES})
		string(REPLACE "-l" "" libName "${lldLib}")
		list(APPEND LLD_LIB_PATHS "${LLVM_ROOT_DIR}/lib/lib${libName}.a")
	endforeach()

	# icui18n icuio icutu

	set(LLVM_LIB_PATHS ${LLVM_LIB_PATHS})
	target_link_libraries(${PROJECT_NAME}
	  ${LLVM_LIB_PATHS} ${LLD_LIB_PATHS}
	  QAT_AST QAT_LEXER QAT_PARSER QAT_IR QAT_UTILS QAT_CLI clangBasic
	  "${CMAKE_SYSROOT}/usr/lib/libc++.tbd" "${LIBS_DIR}/zlib/lib/libz.a"
	  "${LIBS_DIR}/zstd/lib/libzstd.a" "${LIBS_DIR}/ncurses/lib/libcurses.a"
	  "${LIBS_DIR}/libxml2/lib/libxml2.a" "${LIBS_DIR}/libiconv/lib/libiconv.a"
	  "${LIBS_DIR}/xz/lib/liblzma.a"	"${LIBS_DIR}/icu4c/lib/libicuuc.a"
	  "${LIBS_DIR}/icu4c/lib/libicudata.a")
elseif(is_os_windows)
	string(REPLACE " " ";" LLVM_LIBRARIES "${LLVM_LIBRARIES}")
	message(STATUS "Linking libraries ${LLVM_LIBRARIES}")
	if (is_runtime_mingw)
	  target_link_libraries(${PROJECT_NAME}
		QAT_UTILS QAT_CLI QAT_AST QAT_LEXER QAT_PARSER QAT_IR
		${LLVM_LIBRARIES} ${LLD_LIBRARIES} clangBasic pthread "${LIBS_DIR}/libxml2/lib/libxml2.a")
   else()
	 target_link_libraries(${PROJECT_NAME}
		QAT_UTILS QAT_CLI QAT_AST QAT_LEXER QAT_PARSER QAT_IR
		${LLVM_LIBRARIES} ${LLD_LIBRARIES} clangBasic)
	endif()
endif()

message(STATUS "CMake installation prefix is: ${CMAKE_INSTALL_PREFIX}")

if (is_os_linux)
  if (NOT CMAKE_CROSSCOMPILING)
    include(GNUInstallDirs)
  endif()
endif()

# Installing all files
if (is_os_windows)
  set(BIN_EXT ".exe")
endif()

install(TARGETS "${PROJECT_NAME}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

string(REPLACE "." ";" LLVM_VERSION_LIST ${LLVM_VERSION})
list(GET LLVM_VERSION_LIST 0 CLANG_VERSION)

set(CLANG_PATH "${LLVM_ROOT_DIR}/bin/clang-${CLANG_VERSION}${BIN_EXT}")
if (NOT EXISTS CLANG_PATH)
  set(CLANG_PATH "${LLVM_ROOT_DIR}/bin/clang${BIN_EXT}")
endif()

install(FILES "${CLANG_PATH}" TYPE "BIN" RENAME "qat-clang${BIN_EXT}"
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
install(FILES "${LLVM_ROOT_DIR}/bin/llvm-ar${BIN_EXT}" TYPE "BIN" RENAME "qat-llvm-ar${BIN_EXT}"
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
install(FILES "${LLVM_ROOT_DIR}/bin/lld${BIN_EXT}" TYPE "BIN"
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
file(GLOB FILES_TO_REMOVE "${CMAKE_SOURCE_DIR}/std/**/*.qat~")
foreach (file ${FILES_TO_REMOVE})
  file(REMOVE ${file})
endforeach()
install(DIRECTORY "${CMAKE_SOURCE_DIR}/std/" DESTINATION "std")
install(DIRECTORY DESTINATION "toolchain")

# CPack for bundling the release files
set(CPACK_GENERATOR "ZIP")
set(CPACK_VERBATIM_VARIABLES YES)

if(is_os_macos)
	set(SYSTEM_NAME_LOWERCASE "macos")
else()
	string(TOLOWER "${CMAKE_SYSTEM_NAME}" SYSTEM_NAME_LOWERCASE)
endif()

string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" SYSTEM_PROCESSOR_LOWERCASE)
set(BUNDLE_FILE_NAME "qat-${PROJECT_VERSION}-${SYSTEM_NAME_LOWERCASE}-${SYSTEM_PROCESSOR_LOWERCASE}")
message(STATUS "Bundle file name is ${BUNDLE_FILE_NAME}")
set(CPACK_PACKAGE_FILE_NAME "${BUNDLE_FILE_NAME}")
set(CPACK_ARCHIVE_THREADS 0)
include(CPack)
