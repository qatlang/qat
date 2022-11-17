#include "./qat_sitter.hpp"
#include "memory_tracker.hpp"

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

int main(int count, const char** args) {
  using namespace qat;

#if PLATFORM_IS_WINDOWS
  setTerminalColors();
#endif

  auto* cli = cli::Config::init(count, args);
  if (cli->shouldExit()) {
    return 0;
  }
  auto* sitter = new QatSitter();
  sitter->init();
  SHOW("QatSitter initted")
  delete cli::Config::get();
  delete sitter;
  MemoryTracker::report();
  return 0;
}
