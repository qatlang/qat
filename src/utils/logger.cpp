#include "./logger.hpp"
#include "../cli/color.hpp"
#include <array>
#include <atomic>
#include <iostream>
#include <thread>
#include <variant>

namespace qat {

Maybe<Unique<Logger>> Logger::instance = None;

std::array<String, 10> Logger::progressChars = {"[     ]", "[+    ]", "[++   ]", "[+++  ]", "[++++ ]",
                                                "[+++++]", "[ ++++]", "[  +++]", "[   ++]", "[    +]"};

Logger::Logger() {
  updateThread = std::thread([&]() {
    bool lastRun = false;
    while (true) {
      if (mustUpdate.load(std::memory_order_acquire) || showBuffer) {
        updateBusy.lock();
        if (stopThread.load(std::memory_order_acquire)) {
          updateBusy.unlock();
          std::this_thread::sleep_for(std::chrono::milliseconds(20));
          while (!updateBusy.try_lock()) {
          }
          lastRun = true;
        }
        if (!lines.empty()) {
          if (hadPersistentPreviously) {
            std::cout << EraseLineAndGoToStartOfLine;
          }
          for (auto& qLine : lines) {
            std::cout << qLine << "\n";
          }
          lines.clear();
          if (persistent.has_value()) {
            if (showBuffer) {
              std::cout << progressChars[bufferState] << " ";
              if ((bufferState + 1) < progressChars.size()) {
                bufferState++;
              } else {
                bufferState = 0u;
              }
            }
            std::cout << persistent.value() << std::flush;
            hadPersistentPreviously = true;
          } else {
            hadPersistentPreviously = false;
          }
        } else {
          if (hadPersistentPreviously) {
            std::cout << EraseLineAndGoToStartOfLine;
          }
          if (persistent.has_value()) {
            if (showBuffer) {
              std::cout << progressChars[bufferState] << " ";
              if ((bufferState + 1) < progressChars.size()) {
                bufferState++;
              } else {
                bufferState = 0u;
              }
            }
            std::cout << persistent.value() << std::flush;
            hadPersistentPreviously = true;
          } else {
            hadPersistentPreviously = false;
          }
        }
        mustUpdate.store(false, std::memory_order_release);
        updateBusy.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
      }
      if (lastRun) {
        break;
      }
    }
  });
  updateThread.detach();
}

Unique<Logger>& Logger::get() {
  if (!instance.has_value()) {
    instance = std::make_unique<Logger>();
  }
  return instance.value();
}

void Logger::setPersistent(String pers) {
  std::thread(
      [&](String value) {
        while (!updateBusy.try_lock()) {
        }
        persistent = value;
        mustUpdate.store(true, std::memory_order_release);
        updateBusy.unlock();
      },
      std::move(pers))
      .detach();
}

void Logger::resetPersistent(bool erasePersistent) {
  std::thread([&]() {
    while (!updateBusy.try_lock()) {
    }
    if (persistent.has_value()) {
      std::cout << (erasePersistent ? EraseLineAndGoToStartOfLine : "\n");
    }
    persistent = None;
    mustUpdate.store(true, std::memory_order_release);
    updateBusy.unlock();
  }).detach();
}

void Logger::say(String message) {
  std::thread(
      [&](String value) {
        while (!updateBusy.try_lock()) {
        }
        lines.push_back(value);
        mustUpdate.store(true, std::memory_order_release);
        updateBusy.unlock();
      },
      std::move(message))
      .detach();
}

void Logger::enableBuffering() {
  std::thread([&]() {
    while (!updateBusy.try_lock()) {
    }
    showBuffer = true;
    if (persistent.has_value()) {
      mustUpdate.store(true, std::memory_order_release);
    }
    updateBusy.unlock();
  }).detach();
}

void Logger::disableBuffering() {
  std::thread([&]() {
    while (!updateBusy.try_lock()) {
    }
    showBuffer = false;
    mustUpdate.store(true, std::memory_order_release);
    updateBusy.unlock();
  }).detach();
}

void Logger::warn(String message, Maybe<ErrorLocation> range) {
  while (!updateBusy.try_lock()) {
  }
  std::cerr << (hadPersistentPreviously ? "\n" : "") << colors::highIntensityBackground::purple << " WARNING "
            << colors::reset
            << (range.has_value() ? (String(colors::cyan) + " --> " + colors::reset +
                                     getPathFromErrorLocation(range.value()).string() +
                                     (std::holds_alternative<FileRange>(range.value())
                                          ? (":" + std::to_string(std::get<FileRange>(range.value()).start.line) + ":" +
                                             std::to_string(std::get<FileRange>(range.value()).start.character))
                                          : "") +
                                     "\n")
                                  : "")
            << colors::white << message << colors::reset << std::endl;
  updateBusy.unlock();
}

void Logger::fatalError(String message, Maybe<ErrorLocation> range) {
  stopThread.store(true, std::memory_order_release);
  while (!updateBusy.try_lock()) {
  }
  std::cerr << (hadPersistentPreviously ? "\n" : "") << colors::highIntensityBackground::red << " ERROR "
            << colors::reset
            << (range.has_value() ? (String(colors::cyan) + " --> " + colors::reset +
                                     getPathFromErrorLocation(range.value()).string() +
                                     (std::holds_alternative<FileRange>(range.value())
                                          ? (":" + std::to_string(std::get<FileRange>(range.value()).start.line) + ":" +
                                             std::to_string(std::get<FileRange>(range.value()).start.character))
                                          : "") +
                                     "\n")
                                  : "")
            << colors::white << message << colors::reset << std::endl;
  updateBusy.unlock();
  std::exit(1);
}

Logger::~Logger() {
  while (!updateBusy.try_lock()) {
  }
  stopThread.store(true, std::memory_order_release);
  updateBusy.unlock();
  while (stopThread.load(std::memory_order_acquire) && !updateBusy.try_lock()) {
  }
  updateBusy.unlock();
}

} // namespace qat