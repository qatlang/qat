<div>
[![Buy me a coffee](https://img.shields.io/badge/Buy%20Me%20a%20Coffee-ffdd00?style=for-the-badge&logo=buy-me-a-coffee&logoColor=black)](https://buymeacoffee.com/aldrinmathew) [![Open Collective](https://img.shields.io/badge/Open%20Collective-3385FF?style=for-the-badge&logo=open-collective&logoColor=white)](https://opencollective.com/qatlang)
[![Ko-Fi](https://img.shields.io/badge/Ko--fi-F16061?style=for-the-badge&logo=ko-fi&logoColor=white)](https://ko-fi.com/aldrinmathew)
[![PayPal](https://img.shields.io/badge/PayPal-00457C?style=for-the-badge&logo=paypal&logoColor=white)](https://paypal.me/aldrinsartfactory)
</div>

![Qat cover image](./media/qat_curved_cover.png)

<div>
[![Qat Website](https://img.shields.io/badge/qatlang.org-444444?style=for-the-badge&logoColor=white)](https://qatlang.org)
[![YouTube](https://img.shields.io/badge/YouTube-FF0000?style=for-the-badge&logo=youtube&logoColor=white)](https://youtube.com/@aldrinmathew)
[![Discord](https://img.shields.io/badge/Discord-7289DA?style=for-the-badge&logo=discord&logoColor=white)](https://discord.gg/CNW3Uvptvd)
[![Reddit](https://img.shields.io/badge/Reddit-FF4500?style=for-the-badge&logo=reddit&logoColor=white)](https://reddit.com/r/qatlang)
[![Github Profile](https://img.shields.io/badge/Profile-000000?style=for-the-badge&logo=github&logoColor=white)](https://github.com/AldrinMathew)
<hr>
</div>

**qat** is closer to your machine's heart. A superfast, modern systems language for reliable & maintainable code...

### Focus-points of `qat`

- Changes in data are obvious by design
- Advanced compile-time execution & configuration
- Safe pointer types
- Built-in memory management features
- Expressive & flexible syntax
- Simplistic approach to systems programming
- Modular & customisable build process

If you are facing issues with the language, [create an issue](https://github.com/qatlang/qat/issues/new/choose).

### Building the project

Requirements:

- CMake 3.16.3 minimum - Latest recommended
- LLVM 19 (llvm + lld + clang) - Static libraries
- Boost 1.86 (Boost.Filesystem) - Static libraries

Make sure that the LLVM & Boost builds are in release mode, if you are building those yourself.

```bash
cmake -DCMAKE_INSTALL_PREFIX="${HOME}/dev/qat" -DCMAKE_BUILD_TYPE=Release -DLLVM_DIR="/path/to/llvm" -DCMAKE_CXX_COMPILER="clang++-19" -DBOOST_DIR="/path/to/boost" -DCMAKE_C_COMPILER="clang-19" -DCMAKE_C_COMPILER_WORKS=1 -DCMAKE_CXX_COMPILER_WORKS=1 -DBUILD_SHARED_LIBS=false -GNinja -S src/ -B build/
cmake --build build --config Release --target install
```

The above command installs the language in the directory `${HOME}/dev/qat`, which is the recommended installation path for unix-like systems. Feel free to make changes to the above configuration to suit your needs.

### License

This project is published under the [**Public Source Licence**](https://github.com/qatlang/qat/blob/main/LICENSE) and is solely maintained by [Aldrin Mathew](https://github.com/AldrinMathew).
