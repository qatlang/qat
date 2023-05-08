[![Wakatime](https://wakatime.com/badge/user/af510812-c60b-4a16-bb6e-fada8313362b/project/e1c4e435-cfac-41ac-9ba3-59d61be2f357.svg)](https://qat.dev)

![Qat cover image](./media/cover_wide.png)

<div><center>
<a href="https://qat.dev" target="_blank" rel="noopener noreferrer"><img src="https://img.shields.io/badge/qat.dev-444444?style=for-the-badge&logoColor=white"/></a>
<a href="https://youtube.com/c/aldrinmathew" target="_blank" rel="noopener noreferrer"><img src="https://img.shields.io/badge/YouTube-FF0000?style=for-the-badge&logo=youtube&logoColor=white"/></a>
<a href="https://discord.gg/CNW3Uvptvd" target="_blank" rel="noopener noreferrer"><img src="https://img.shields.io/badge/Discord-7289DA?style=for-the-badge&logo=discord&logoColor=white"/></a>
<a href="https://reddit.com/r/qatlang" target="_blank" rel="noopener noreferrer"><img src="https://img.shields.io/badge/Reddit-FF4500?style=for-the-badge&logo=reddit&logoColor=white"/></a>
<a href="https://github.com/AldrinMathew" target="_blank" rel="noopener noreferrer"><img src="https://img.shields.io/badge/Profile-000000?style=for-the-badge&logo=github&logoColor=white"/></a>
<hr>
</div>

**qat** is closer to your machine's heart. It is envisioned to be a superfast, legible systems language for maintainable code...

This project is published under a **Shared Source** [license](https://github.com/qatlang/qat/blob/main/LICENSE) and is solely maintained by <a href="https://github.com/AldrinMathew" target="_blank" rel="noopener noreferrer">Aldrin Mathew</a>. The project is not open to public contributions when it comes to development. But if you are facing issues with the language, you are always welcome to [open an issue](https://github.com/qatlang/qat/issues/new/choose).

Besides that, you can contribute to the project via the following links:

<div><center>
<a href="https://ko-fi.com/aldrinmathew" target="_blank" rel="noopener noreferrer"><img src="https://img.shields.io/badge/Ko--fi-F16061?style=for-the-badge&logo=ko-fi&logoColor=white"/></a>
<a href="https://paypal.me/aldrinsartfactory" target="_blank" rel="noopener noreferrer"><img src="https://img.shields.io/badge/PayPal-00457C?style=for-the-badge&logo=paypal&logoColor=white"/></a>
</center></div>


## Warning!!
The project is under heavy development and is not suitable for production use (that is if you can even compile the whole language). Updates will be provided appropriately via the social channels and via Github.

# Building the compiler

## On Windows

```bat
RMDIR /S /Q "./build"
MKDIR build
cmake -DCMAKE_BUILD_TYPE=Release -DPLATFORM=Windows -DLLVM_DIR="/path/to/llvm/build/install/dir" -G"Visual Studio 17 2022" -S ./src/ -B ./build/
CD ./build/
MSBuild qat.sln /m /p:Configuration=Release
CD ..
```

## On Linux

```bash
rm -rf build
mkdir build
cmake -DCMAKE_BUILD_TYPE=Release -DPLATFORM=${1:-Linux} -DLLVM_DIR="/path/to/llvm/build/install/dir" -DCMAKE_CXX_COMPILER="path/to/clang/compiler" -DBUILD_SHARED_LIBS=false -GNinja -S src/ -B build/
cd build
ninja
cd ..
```