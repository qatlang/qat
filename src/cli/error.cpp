#include "./error.hpp"
#include "../memory_tracker.hpp"
#include "color.hpp"
#include "config.hpp"

namespace qat::cli {

#define OnColorMode(col) (noColors ? "" : col)

void Error(const String& message, Maybe<fs::path> path) {
  std::cout << " cli error :: " << (path ? (" " + path.value().string()) : "") << " " << message << std::endl;
  delete cli::Config::get();
  exit(1);
}

void Warning(const String& message, Maybe<fs::path> path) {
  std::cout << " cli warning :: " << (path ? (" " + path.value().string()) : "") << " " << message << std::endl;
}

} // namespace qat::cli