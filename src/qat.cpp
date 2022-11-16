#include "./qat_sitter.hpp"

#if PLATFORM_IS_WINDOWS
#include <iostream>
#ifdef _WIN32
#include <windows.h>
#endif

void setTerminalColors() {
  HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD  consoleMode;
  GetConsoleMode(handle, &consoleMode);
  consoleMode |= 0x0004;
  consoleMode |= 0x0008;
  SetConsoleMode(handle, consoleMode);
}
#endif

int main(int count, char* args[]) {
  using qat::cli::Config;

#if PLATFORM_IS_WINDOWS
  setTerminalColors();
#endif

  auto* cli = Config::init(count, args);
  if (cli->shouldExit()) {
    return 0;
  }
  auto sitter = qat::QatSitter();
  sitter.init();
  Config::destroy();
  return 0;
}
