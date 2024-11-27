#!/bin/bash
clear
rm -rf build/*
cmake -DCMAKE_TOOLCHAIN_FILE="/home/aldrinmathew/toolchain/linux-aarch64.cmake" \
   -DCMAKE_BUILD_TYPE=Release -DLIBS_DIR="/mnt/main/libs/linux-aarch64" \
	-DLLVM_DIR="/mnt/main/libs/linux-aarch64/llvm" \
	-DBUILD_SHARED_LIBS=false -GNinja -S src/ -B build/
cmake --build build --config Release --parallel 24 --target package
