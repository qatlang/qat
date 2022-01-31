#ifndef QAT_UTILITIES_VERSION_NUMBER_HPP
#define QAT_UTILITIES_VERSION_NUMBER_HPP

#include "llvm/ADT/Optional.h"
#include <sstream>
#include <string>

namespace qat {
namespace utilities {
class VersionNumber {
  VersionNumber(const std::string version);

  VersionNumber(const unsigned int _major, const unsigned int _minor,
                const unsigned int _patch, std::string _prerelease,
                std::string _build)
      : major(_major), minor(_minor), patch(_patch), prerelease(_prerelease),
        build(_build) {}

  VersionNumber(const unsigned int _major, const unsigned int _minor,
                const unsigned int _patch, std::string _prerelease)
      : major(_major), minor(_minor), patch(_patch), prerelease(_prerelease),
        build(llvm::None) {}

  VersionNumber(const unsigned int _major, const unsigned int _minor,
                const unsigned int _patch)
      : major(_major), minor(_minor), patch(_patch), prerelease(llvm::None),
        build(llvm::None) {}

  VersionNumber(const unsigned int _major, const unsigned int _minor)
      : major(_major), minor(_minor), patch(llvm::None), prerelease(llvm::None),
        build(llvm::None) {}

  unsigned int major;
  unsigned int minor;
  llvm::Optional<unsigned int> patch;
  llvm::Optional<std::string> prerelease;
  llvm::Optional<std::string> build;
};
} // namespace utilities
} // namespace qat

#endif