cls
cmake --build build/ --target clean
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="C:/Users/aldri/Development/qat" ^
	-DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DLLVM_DIR="H:/LIBS/llvm-win-release/" ^
	-G"Visual Studio 17 2022" -S ./src/ -B ./build/
cmake --build build --config Release --parallel 24 --target package
