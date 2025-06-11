cls
rmdir /s /q build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="C:/Users/aldri/Development/qat" ^
	-DLLVM_DIR="D:/libs/windows-arm64/llvm" -DBOOST_DIR="D:/libs/windows-arm64/boost" ^
	-DLIBS_DIR="D:/libs/windows-arm64" -G"Visual Studio 17 2022" -DTARGET_ARCHITECTURE="arm64" ^
	-A ARM64 -S ./src/ -B ./build/
cmake --build build --config Release --parallel 24 --target package
