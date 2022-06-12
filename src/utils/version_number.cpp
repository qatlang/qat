/**
 * Qat Programming Language : Copyright 2022 : Aldrin Mathew
 *
 * AAF INSPECTABLE LICENCE - 1.0
 *
 * This project is licensed under the AAF Inspectable Licence 1.0.
 * You are allowed to inspect the source of this project(s) free of
 * cost, and also to verify the authenticity of the product.
 *
 * Unless required by applicable law, this project is provided
 * "AS IS", WITHOUT ANY WARRANTIES OR PROMISES OF ANY KIND, either
 * expressed or implied. The author(s) of this project is not
 * liable for any harms, errors or troubles caused by using the
 * source or the product, unless implied by law. By using this
 * project, or part of it, you are acknowledging the complete terms
 * and conditions of licensing of this project as specified in AAF
 * Inspectable Licence 1.0 available at this URL:
 *
 * https://github.com/aldrinsartfactory/InspectableLicence/
 *
 * This project may contain parts that are not licensed under the
 * same licence. If so, the licences of those parts should be
 * appropriately mentioned in those parts of the project. The
 * Author MAY provide a notice about the parts of the project that
 * are not licensed under the same licence in a publicly visible
 * manner.
 *
 * You are NOT ALLOWED to sell, or distribute THIS project, its
 * contents, the source or the product or the build result of the
 * source under commercial or non-commercial purposes. You are NOT
 * ALLOWED to revamp, rebrand, refactor, modify, the source, product
 * or the contents of this project.
 *
 * You are NOT ALLOWED to use the name, branding and identity of this
 * project to identify or brand any other project. You ARE however
 * allowed to use the name and branding to pinpoint/show the source
 * of the contents/code/logic of any other project. You are not
 * allowed to use the identification of the Authors of this project
 * to associate them to other projects, in a way that is deceiving
 * or misleading or gives out false information.
 */

#include "./version_number.hpp"

namespace qat {
namespace utils {

VersionNumber::VersionNumber(const std::string version)
    : major(0), minor(0), patch(std::nullopt), build(std::nullopt) {
  const bool hasV = (version[0] == 'v');
  std::vector<std::string> nums;
  std::string cache;
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
    major = std::stoi(nums.at(0));
    if (nums.size() < 2) {
      minor = 0;
      patch = std::nullopt;
      build = std::nullopt;
    } else {
      minor = std::stoi(nums.at(1));
      if (nums.size() < 3) {
        patch = std::nullopt;
        prerelease = std::nullopt;
        build = std::nullopt;
      } else {
        patch = std::stoi(nums.at(2));
        if (nums.size() < 4) {
          prerelease = std::nullopt;
          build = std::nullopt;
        } else {
          prerelease = nums.at(3);
          if (nums.size() < 5) {
            build = std::nullopt;
          } else {
            build = nums.at(4);
          }
        }
      }
    }
  }
}

} // namespace utils
} // namespace qat