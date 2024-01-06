#ifndef QAT_LOGGER_HPP
#define QAT_LOGGER_HPP

#include "file_range.hpp"
#include "helpers.hpp"
#include <syncstream>
#include <variant>

#define EraseLineAndGoToStartOfLine "\33[2K\r"
namespace qat {

namespace cli {
class Config;
}

using ErrorLocation = std::variant<FileRange, fs::path>;

useit inline fs::path getPathFromErrorLocation(ErrorLocation& loc) {
  if (loc.index() == 0) {
    return std::get<FileRange>(loc).file;
  } else {
    return std::get<fs::path>(loc);
  }
}

enum class LogLevel { NONE, VERBOSE };

class Logger {
  friend cli::Config;

  static Maybe<Unique<Logger>> instance;

  LogLevel                 logLevel = LogLevel::NONE;
  mutable std::osyncstream out;
  mutable std::osyncstream errOut;

public:
  Logger();
  ~Logger() = default;
  useit static Unique<Logger> const& get();

  inline void say(String message) const {
    if (logLevel == LogLevel::NONE) {
      return;
    }
    out << message.append("\n");
  }

  void diagnostic(String message) const;
  void warn(String message, Maybe<ErrorLocation> range) const;

  exitFn void fatalError(String message, Maybe<ErrorLocation> range) const;
};

} // namespace qat

#endif