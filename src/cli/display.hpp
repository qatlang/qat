#ifndef QAT_CLI_DISPLAY_HPP
#define QAT_CLI_DISPLAY_HPP

#include "../utils/helpers.hpp"
#include <iostream>
#include <string>

namespace qat::cli::display {

// Displays version of the compiler, in detail
void detailedVersion(String const& buildCommit);

// Short version
void shortVersion();

// Displays details about the qat project like website...
void about();

// Displays the build info
void build_info(const String& buildCommit);

// Displays detailed information on how to use the CLI of the compiler.
void help();

// Displays websites related to the project
void websites();

} // namespace qat::cli::display

#endif