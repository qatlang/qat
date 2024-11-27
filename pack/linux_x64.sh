#!/bin/bash
clear
rm -rf build/*
cmake -DCMAKE_INSTALL_PREFIX="${HOME}/dev/qat" -DCMAKE_BUILD_TYPE=Release \
	-DLLVM_DIR="/mnt/main/libs/llvm-release" -DCMAKE_CXX_COMPILER="/usr/bin/clang++-15" \
	-DCMAKE_C_COMPILER="/usr/bin/clang-15" -DBUILD_SHARED_LIBS=false \
	-GNinja -S src/ -B build/
cmake --build build --config Release --parallel 24 --target package
