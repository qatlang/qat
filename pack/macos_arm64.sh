#!/bin/bash
clear
rm -rf build/*
cmake -DCMAKE_TOOLCHAIN_FILE="/home/aldrinmathew/toolchain/macos-arm64.cmake" \
		-DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER_WORKS=1 -DCMAKE_CXX_COMPILER_WORKS=1 \
		-DLIBS_DIR="/mnt/main/libs/macos-arm64" -DLLVM_DIR="/mnt/main/libs/macos-arm64/llvm" \
		-DBUILD_SHARED_LIBS=false -GNinja -S src/ -B build/
cmake --build build --config Release --parallel 24 --target package
