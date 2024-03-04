#include "./logger.hpp"
#include "../cli/color.hpp"
#include "../cli/config.hpp"
#include <iostream>
#include <variant>

namespace qat {

Maybe<Unique<Logger>> Logger::instance = None;

Logger::Logger() : out(std::cout), errOut(std::cerr) {}

Unique<Logger> const& Logger::get() {
  if (!instance.has_value()) {
    instance = std::make_unique<Logger>();
  }
  return instance.value();
}

void Logger::diagnostic(String message) const {
  if ((logLevel == LogLevel::NONE) && !cli::Config::get()->should_do_diagnostics()) {
    return;
  }
  out << message.append("\n");
}

void Logger::warn(String message, Maybe<ErrorLocation> range) const {
  out << String(colors::highIntensityBackground::purple) + " WARNING " + colors::reset +
             (range.has_value()
                  ? (String(colors::cyan) + " --> " + colors::reset + getPathFromErrorLocation(range.value()).string() +
                     (std::holds_alternative<FileRange>(range.value())
                          ? (":" + std::to_string(std::get<FileRange>(range.value()).start.line) + ":" +
                             std::to_string(std::get<FileRange>(range.value()).start.character))
                          : "") +
                     "\n")
                  : "") +
             colors::white + message + colors::reset + "\n";
}

void Logger::fatalError(String message, Maybe<ErrorLocation> range) const {
  errOut << String(colors::highIntensityBackground::red) + " ERROR " + colors::reset +
                (range.has_value() ? (String(colors::cyan) + " --> " + colors::reset +
                                      getPathFromErrorLocation(range.value()).string() +
                                      (std::holds_alternative<FileRange>(range.value())
                                           ? (":" + std::to_string(std::get<FileRange>(range.value()).start.line) +
                                              ":" + std::to_string(std::get<FileRange>(range.value()).start.character))
                                           : "") +
                                      "\n")
                                   : "") +
                colors::white + message + colors::reset + "\n";
  std::exit(1);
}

} // namespace qat