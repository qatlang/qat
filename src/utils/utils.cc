#include "./find_executable.hpp"
#include "./run_command.hpp"
#include "./unique_id.hpp"
#include <cstdio>
#include <random>

#if MINGW_RUNTIME
#include <sdkddkver.h>
#include <windows.h>
#elif MSVC_RUNTIME
#include <SDKDKKVer.h>
#include <Windows.h>
#endif

namespace qat {

namespace utils {

String unique_id() {
  char               hex_vals[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
  String             result;
  std::random_device dev1;
  std::random_device dev2;
  std::random_device dev3;
  std::mt19937_64    rng1(dev1());
  std::mt19937_64    rng2(dev2());
  std::mt19937_64    rng3(dev3());
  std::uniform_int_distribution<std::mt19937_64::result_type> dist1(0, 15);
  std::uniform_int_distribution<std::mt19937_64::result_type> dist2(0, 15);
  std::uniform_int_distribution<std::mt19937_64::result_type> switch_dist(0, 1);
  for (usize i = 0; i < 32; i++) {
    result += hex_vals[(switch_dist(rng3) == 1) ? dist2(rng2) : dist1(rng1)];
  }
  return result;
}

} // namespace utils

Maybe<String> find_executable(StringView name) {
  const StringView path = std::getenv("PATH");
#if OS_IS_WINDOWS
  const StringView pathExt = std::getenv("PATHENV");

  Vec<StringView> extensions(15);
  usize           i = 0;
  while (i < pathExt.size()) {
    auto sep = pathExt.find_first_of(';', i);
    if (sep != StringView::npos) {
      extensions.push_back(pathExt.substr(i, sep - i));
    } else {
      extensions.push_back(pathExt.substr(i));
    }
    if (sep != StringView::npos) {
      i = sep + 1;
    } else {
      i = pathExt.size();
    }
  }

  i = 0;
  while (i < path.size()) {
    fs::path it;
    auto     sep = path.find_first_of(';', i);
    if (sep != StringView::npos) {
      it = path.substr(i, sep - i);
      i  = sep + 1;
    } else {
      it = path.substr(i);
      i  = path.size();
    }
    for (auto& ext : extensions) {
      auto cand = it / (String(name).append(ext));
      if (fs::exists(cand)) {
        return cand.string();
      }
    }
  }
  if (name != "where") {
    auto wherePath = find_executable("where");
    if (wherePath.has_value()) {
      auto res = run_command_get_output(wherePath.value(), {String(name)});
      if (not res.has_value()) {
        return None;
      }
      if (res->first == 0) {
        if (res->second.ends_with('\n')) {
          res->second.pop_back();
        }
        if (fs::is_regular_file(res->second)) {
          return res->second;
        }
      }
    }
  }
  return None;
#else
  usize i = 0;
  while (i < path.size()) {
    fs::path it;
    auto     colon = path.find_first_of(':', i);
    if (colon != StringView::npos) {
      it = path.substr(i, colon - i);
      i  = colon + 1;
    } else {
      it = path.substr(i);
      i  = path.size();
    }
    auto cand = it / name;
    if (fs::exists(cand)) {
      return cand.string();
    }
  }
  if (name != "which") {
    auto whichPath = find_executable("which");
    if (whichPath.has_value()) {
      auto res = run_command_get_output(whichPath.value(), {String(name)});
      if (not res.has_value()) {
        return None;
      }
      if (res->first == 0) {
        if (res->second.ends_with('\n')) {
          res->second.pop_back();
        }
        if (fs::is_regular_file(res->second)) {
          return res->second;
        }
      }
    }
  }
  return None;
#endif
}

} // namespace qat
