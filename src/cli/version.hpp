#ifndef QAT_CLI_VERSION_HPP
#define QAT_CLI_VERSION_HPP

#include <string>

#define STRINGIFY(a) str_val(a)

#define str_val(a) #a

#define BUILD_TYPE STRINGIFY(QAT_BUILD_TYPE)

#define BUILD_COMMIT_QUOTED STRINGIFY(QAT_BUILD_ID)

#define BUILD_BRANCH STRINGIFY(QAT_BUILD_BRANCH)

#define VERSION_STRING                                                         \
  std::string(STRINGIFY(QAT_VERSION)) +                                        \
      (QAT_IS_PRERELEASE ? "-" STRINGIFY(QAT_PRERELEASE) : "")

#endif