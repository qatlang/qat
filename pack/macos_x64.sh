#!/bin/bash
clear
rm -rf build/*
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="/home/aldrinmathew/toolchain/macos-x64.cmake" \
	-DLIBS_DIR="/mnt/main/libs/macos-x64" -DCMAKE_C_COMPILER_WORKS=1 -DCMAKE_CXX_COMPILER_WORKS=1 \
	-DBOOST_INCLUDE_DIR="/mnt/main/libs/macos-x64/boost/include/" -DLLVM_DIR="/mnt/main/libs/macos-x64/llvm" \
	-DBUILD_SHARED_LIBS=false -GNinja -S src/ -B build/
cmake --build build --config Release --parallel 24 --target package
