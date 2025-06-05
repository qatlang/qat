#!/bin/bash
clear
rm -rf build/*
cmake -DCMAKE_TOOLCHAIN_FILE="~/toolchains/linux-aarch64.cmake" \
	-DCMAKE_BUILD_TYPE=Release -DLIBS_DIR="/mnt/Core/libs/linux-aarch64" \
	-DLLVM_DIR="/mnt/Core/libs/linux-aarch64/llvm" \
	-DBOOST_DIR="/mnt/Core/libs/linux-aarch64/boost" \
	-DCMAKE_CXX_FLAGS="-fuse-ld=lld" \
	-DBUILD_SHARED_LIBS=false -GNinja -S src/ -B build/
cmake --build build --config Release --parallel 24 --target package
