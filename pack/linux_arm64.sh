#!/bin/bash
clear
rm -rf build/*
cmake -DCMAKE_BUILD_TYPE=Release -DLLVM_DIR="$HOME/dev/llvm" \
		-DCMAKE_C_COMPILER="/usr/bin/clang" -DCMAKE_CXX_COMPILER="/usr/bin/clang++" \
		-DBUILD_SHARED_LIBS=false -GNinja -S src/ -B build/
cmake --build build --config Release --parallel 4 --target package
