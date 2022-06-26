#ifndef QAT_UTILS_VERSION_NUMBER_HPP
#define QAT_UTILS_VERSION_NUMBER_HPP

#include <optional>
#include <string>
#include <vector>

namespace qat {
namespace utils {
class VersionNumber {
public:
  VersionNumber(const std::string version);

  VersionNumber(const unsigned int _major, const unsigned int _minor,
                const unsigned int _patch, const std::string _prerelease,
                const std::string _build)
      : major(_major), minor(_minor), patch(_patch), prerelease(_prerelease),
        build(_build) {}

  VersionNumber(const unsigned int _major, const unsigned int _minor,
                const unsigned int _patch, const std::string _prerelease)
      : major(_major), minor(_minor), patch(_patch), prerelease(_prerelease),
        build(std::nullopt) {}

  VersionNumber(const unsigned int _major, const unsigned int _minor,
                const unsigned int _patch)
      : major(_major), minor(_minor), patch(_patch), prerelease(std::nullopt),
        build(std::nullopt) {}

  VersionNumber(const unsigned int _major, const unsigned int _minor)
      : major(_major), minor(_minor), patch(std::nullopt),
        prerelease(std::nullopt), build(std::nullopt) {}

  unsigned int major;
  unsigned int minor;
  std::optional<unsigned int> patch;
  std::optional<std::string> prerelease;
  std::optional<std::string> build;
};
} // namespace utils
} // namespace qat

#endif