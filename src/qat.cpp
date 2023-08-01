#include "./qat_sitter.hpp"
#include "cli/config.hpp"
#include "memory_tracker.hpp"
#include <iostream>

#if PlatformIsWindows
#ifdef _WIN32
#include <windows.h>
#endif

#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#define DISABLE_NEWLINE_AUTO_RETURN        0x0008

void setTerminalColors(qat::cli::Config* cfg) {
  if (!cfg->noColorMode()) {
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD  consoleMode;
    GetConsoleMode(handle, &consoleMode);
    consoleMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    consoleMode |= DISABLE_NEWLINE_AUTO_RETURN;
    SetConsoleMode(handle, consoleMode);
  }
}
#else
void setTerminalColors(qat::cli::Config* cfg) {}
#endif

int main(int count, const char** args) {
  using namespace qat;

  auto* cli = cli::Config::init(count, args);
  if (cli->shouldExit()) {
    return 0;
  }
  setTerminalColors(cli);
  auto* sitter = new QatSitter();
  sitter->init();
  SHOW("QatSitter initted")
  delete cli::Config::get();
  delete sitter;
  MemoryTracker::report();
  return 0;
}
