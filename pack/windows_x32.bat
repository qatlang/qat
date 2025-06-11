cls
rmdir /s /q build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="C:/Users/aldri/Development/qat" ^
	-DLLVM_DIR="D:/libs/windows-x32/llvm" -DBOOST_DIR="D:/libs/windows-x32/boost" ^
	-DLIBS_DIR="D:/libs/windows-x32" -G"Visual Studio 17 2022" -DTARGET_ARCHITECTURE="x32" ^
	-A Win32 -S ./src/ -B ./build/
cmake --build build --config Release --parallel 24 --target package
