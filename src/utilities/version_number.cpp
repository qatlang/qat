#include "./version_number.hpp"

qat::utilities::VersionNumber::VersionNumber(const std::string version)
    : major(0), minor(0), patch(llvm::None), build(llvm::None) {
  const bool hasV = (version[0] == 'v');
  std::vector<std::string> nums;
  std::string cache = "";
  for (std::size_t i = hasV; i < version.length(); i++) {
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
    std::stringstream majorStr(nums.at(0));
    majorStr >> major;
    if (nums.size() < 2) {
      minor = 0;
      patch = llvm::None;
      build = llvm::None;
    } else {
      std::stringstream minorStr(nums.at(1));
      minorStr >> minor;
      if (nums.size() < 3) {
        patch = llvm::None;
        prerelease = llvm::None;
        build = llvm::None;
      } else {
        std::stringstream patchStr(nums.at(2));
        unsigned int patchNum = 0;
        patchStr >> patchNum;
        patch = patchNum;
        if (nums.size() < 4) {
          prerelease = llvm::None;
          build = llvm::None;
        } else {
          prerelease = nums.at(3);
          if (nums.size() < 5) {
            build = llvm::None;
          } else {
            build = nums.at(4);
          }
        }
      }
    }
  }
}