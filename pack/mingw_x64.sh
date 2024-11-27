#!/bin/bash
clear
rm -rf build/*
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="/home/aldrinmathew/toolchain/windows-x64.cmake" \
	-DWINDOWS_SDK_PATH="/mnt/main/libs/toolchain/sdk/10" -DMSVC_PATH="/mnt/main/libs/toolchain/msvc" \
	-DLIBS_DIR="/mnt/main/libs/windows-x64" -DCMAKE_C_COMPILER_WORKS=1 -DCMAKE_CXX_COMPILER_WORKS=1 \
	# -DBOOST_INCLUDE_DIR="/mnt/main/libs/windows-x64/boost/include/" -DBoost_COMPILER="vc143" \
	-DRUNTIME=mingw -DLLVM_DIR="/mnt/main/libs/windows-x64/llvm-mingw" \
	-DBUILD_SHARED_LIBS=false -GNinja -S src/ -B build/
cmake --build build --config Release --parallel 24 --target package
# -DBoost_USE_MULTITHREADED=TRUE -DBoost_USE_STATIC_LIBS=ON
