#ifndef QAT_CLI_DISPLAY_HPP
#define QAT_CLI_DISPLAY_HPP

#include "../utils/helpers.hpp"
#include <iostream>
#include <string>

namespace qat::cli::display {

// Displays version of the compiler (Short form)
void version();

// Displays details about the compiler, like version build info, website...
void about(const String& buildCommit);

// Displays the build info
void build_info(const String& buildCommit);

// Displays detailed information on how to use the CLI of the compiler.
void help();

// Displays websites related to the project
void websites();

} // namespace qat::cli::display

#endif