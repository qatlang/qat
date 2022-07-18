#ifndef QAT_CLI_DISPLAY_HPP
#define QAT_CLI_DISPLAY_HPP

#include "../utils/types.hpp"
#include "./color.hpp"
#include "./version.hpp"
#include <iostream>
#include <string>

namespace qat::cli::display {

/**
 *  Displays version of the compiler (Short form)
 *
 */
void version();
/**
 *  Displays details about the compiler, like version build info,
 * website...
 *
 * @param buildCommit
 */
void about(std::string buildCommit);

/**
 *  Displays the build info
 *
 * @param buildCommit
 */
void build_info(std::string buildCommit);

/**
 *  Displays detailed information on how to use the CLI of the compiler.
 *
 */
void help();

/**
 *  Displays websites related to the project
 *
 */
void websites();

/**
 *  Displays information about potential errors that are shown while using
 * the language in a wrong manner
 *
 */
void errors();

} // namespace qat::cli::display

#endif