#ifndef QAT_LOGGER_HPP
#define QAT_LOGGER_HPP

#include "../utils/file_range.hpp"
#include "../utils/helpers.hpp"
#include <iostream>
#include <variant>

#define EraseLineAndGoToStartOfLine "\33[2K\r"
namespace qat {
namespace ir {
class Ctx;
}
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
	friend ir::Ctx;

	static Maybe<Unique<Logger>> instance;

	LogLevel logLevel = LogLevel::NONE;

  public:
	Logger();
	~Logger() = default;
	useit static Unique<Logger> const& get();

	void say(String message) const {
		if (logLevel == LogLevel::NONE) {
			return;
		}
		std::cout << message.append("\n");
	}

	String color(String const& message) const;

	void diagnostic(String message) const;
	void warn(String message, Maybe<ErrorLocation> range);

	exitFn void fatalError(String message, Maybe<ErrorLocation> range);
};

} // namespace qat

#endif
