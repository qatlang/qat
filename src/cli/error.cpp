#include "./error.hpp"
#include "../utils/logger.hpp"
#include "color.hpp"
#include "config.hpp"

namespace qat::cli {

#define OnColorMode(col) (noColors ? "" : col)

void Error(const String& message, Maybe<fs::path> path) { Logger::get()->fatalError(message, path); }

void Warning(const String& message, Maybe<fs::path> path) { Logger::get()->warn(message, path); }

} // namespace qat::cli