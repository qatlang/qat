#ifndef QAT_UTILS_VERSION_NUMBER_HPP
#define QAT_UTILS_VERSION_NUMBER_HPP

#include "./helpers.hpp"

namespace qat::utils {

class VersionNumber {
public:
  explicit VersionNumber(const String &version);

  VersionNumber(const u32 _major, const u32 _minor, const u32 _patch,
                const String &_prerelease, const String &_build)
      : major(_major), minor(_minor), patch(_patch), prerelease(_prerelease),
        build(_build) {}

  VersionNumber(const u32 _major, const u32 _minor, const u32 _patch,
                const String &_prerelease)
      : major(_major), minor(_minor), patch(_patch), prerelease(_prerelease),
        build(None) {}

  VersionNumber(const u32 _major, const u32 _minor, const u32 _patch)
      : major(_major), minor(_minor), patch(_patch), prerelease(None),
        build(None) {}

  VersionNumber(const u32 _major, const u32 _minor)
      : major(_major), minor(_minor), patch(None), prerelease(None),
        build(None) {}

  u32           major;
  u32           minor;
  Maybe<u32>    patch;
  Maybe<String> prerelease;
  Maybe<String> build;
};

} // namespace qat::utils

#endif