#include "./version_number.hpp"

namespace qat::utils {

VersionNumber::VersionNumber(const String &version)
    : major(0), minor(0), patch(None), build(None) {
  const bool  hasV = (version[0] == 'v');
  Vec<String> nums;
  String      cache;
  for (usize i = hasV; i < version.length(); i++) {
    if (nums.size() == 3) {
      if (version[i] == '-') {
        cache = "";
        i++;
        while ((version[i] != '+') && (i < version.length())) {
          cache += version[i];
          i++;
        }
        nums.push_back(cache);
        cache = "";
        if (version[i] == '+') {
          i++;
          while (i < version.length()) {
            cache += version[i];
            i++;
          }
        }
      } else {
        nums.push_back("");
      }
    } else {
      if (version[i] == '.') {
        nums.push_back(cache);
        cache = "";
      } else {
        cache += version.at(i);
      }
    }
  }
  if (cache != "") {
    nums.push_back(cache);
  }
  if (nums.size() < 1) {
    major = 0;
    minor = 0;
    patch = 0;
    build = "";
  } else {
    major = std::stoi(nums.at(0));
    if (nums.size() < 2) {
      minor = 0;
      patch = None;
      build = None;
    } else {
      minor = std::stoi(nums.at(1));
      if (nums.size() < 3) {
        patch      = None;
        prerelease = None;
        build      = None;
      } else {
        patch = std::stoi(nums.at(2));
        if (nums.size() < 4) {
          prerelease = None;
          build      = None;
        } else {
          prerelease = String(nums.at(3));
          if (nums.size() < 5) {
            build = None;
          } else {
            build = String(nums.at(4));
          }
        }
      }
    }
  }
}

} // namespace qat::utils