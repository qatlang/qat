#include "./logger.hpp"
#include "./color.hpp"
#include "./config.hpp"
#include <iostream>
#include <variant>

namespace qat {

Maybe<Unique<Logger>> Logger::instance = None;

Logger::Logger() {}

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
  std::cout << message.append("\n");
}

String Logger::color(const String& message) const {
  auto cfg = cli::Config::get();
  return (cfg->is_no_color_mode() ? "`" : String(colors::bold) + cli::get_color(cli::Color::yellow)) + message +
         (cfg->is_no_color_mode() ? "`" : String(colors::reset) + cli::get_color(cli::Color::white));
}

void Logger::warn(String message, Maybe<ErrorLocation> range) {
  std::cout << String(cli::get_bg_color(cli::Color::purple)) + " WARNING " + cli::get_bg_color(cli::Color::reset) +
                   (range.has_value()
                        ? (String(cli::get_color(cli::Color::cyan)) + " --> " + cli::get_color(cli::Color::reset) +
                           getPathFromErrorLocation(range.value()).string() +
                           (std::holds_alternative<FileRange>(range.value())
                                ? (":" + std::to_string(std::get<FileRange>(range.value()).start.line) + ":" +
                                   std::to_string(std::get<FileRange>(range.value()).start.character))
                                : "") +
                           "\n")
                        : "") +
                   cli::get_color(cli::Color::white) + message + cli::get_color(cli::Color::reset) + "\n";
}

void Logger::fatalError(String message, Maybe<ErrorLocation> range) {
  std::cerr << String(cli::get_bg_color(cli::Color::red)) + " ERROR " + cli::get_bg_color(cli::Color::reset) +
                   (range.has_value()
                        ? (" --> " + getPathFromErrorLocation(range.value()).string() +
                           (std::holds_alternative<FileRange>(range.value())
                                ? (":" + std::to_string(std::get<FileRange>(range.value()).start.line) + ":" +
                                   std::to_string(std::get<FileRange>(range.value()).start.character))
                                : "") +
                           "\n")
                        : "") +
                   cli::get_color(cli::Color::white) + message + cli::get_color(cli::Color::reset) + "\n";
  std::exit(1);
}

} // namespace qat