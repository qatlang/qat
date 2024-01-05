#ifndef QAT_LOGGER_HPP
#define QAT_LOGGER_HPP

#include "file_range.hpp"
#include "helpers.hpp"
#include <atomic>
#include <mutex>
#include <thread>
#include <variant>

#define EraseLineAndGoToStartOfLine "\33[2K\r"
namespace qat {

using ErrorLocation = std::variant<FileRange, fs::path>;

useit inline fs::path getPathFromErrorLocation(ErrorLocation& loc) {
  if (loc.index() == 0) {
    return std::get<FileRange>(loc).file;
  } else {
    return std::get<fs::path>(loc);
  }
}

class Logger {
  static Maybe<Unique<Logger>>  instance;
  static std::array<String, 10> progressChars;

  std::thread      updateThread;
  std::atomic_bool stopThread = false;
  std::mutex       updateBusy;
  std::atomic_bool mustUpdate = false;

  Vec<String>   lines;
  bool          hadPersistentPreviously = false;
  Maybe<String> persistent;
  bool          showBuffer  = false;
  usize         bufferState = 0u;

public:
  Logger();
  ~Logger();
  useit static Unique<Logger>& get();

  void enableBuffering();
  void disableBuffering();
  void setPersistent(String pers);
  void resetPersistent(bool erasePersistent);

  void say(String message);

  void        warn(String message, Maybe<ErrorLocation> range);
  exitFn void fatalError(String message, Maybe<ErrorLocation> range);
};

} // namespace qat

#endif