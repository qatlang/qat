#include "./qat_sitter.hpp"

#if PLATFORM_IS_WINDOWS
#include <iostream>
#ifdef _WIN32
#include <windows.h>
#endif
#endif

void setTerminalColors() {
#if PLATFORM_IS_WINDOWS
  HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD consoleMode;
  GetConsoleMode(handleOut, &consoleMode);
  consoleMode |= 0x0004;
  consoleMode |= 0x0008;
  SetConsoleMode(handleOut, consoleMode);
#endif
}

int main(int count, const char **args) {
  using qat::cli::Config;

  setTerminalColors();

  auto cli = Config::init(count, args);
  if (cli->shouldExit()) {
    return 0;
  }
  auto sitter = qat::QatSitter();
  sitter.init();
  Config::destroy();
  return 0;
}
