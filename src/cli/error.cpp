#include "./error.hpp"
#include "../memory_tracker.hpp"
#include "color.hpp"
#include "config.hpp"

namespace qat::cli {

void Error(const String& message, Maybe<fs::path> path) {
  std::cout << colors::highIntensityBackground::red << " cli error " << colors::reset
            << (path ? colors::green : colors::reset) << (path ? (" " + path.value().string()) : "")
            << colors::bold::white << " " << message << colors::reset << std::endl;
  delete cli::Config::get();
  exit(1);
}

void Warning(const String& message, Maybe<fs::path> path) {
  std::cout << colors::highIntensityBackground::purple << " cli warning " << colors::reset
            << (path ? colors::green : colors::reset) << (path ? (" " + path.value().string()) : "")
            << colors::bold::purple << " " << message << colors::reset << std::endl;
}

} // namespace qat::cli