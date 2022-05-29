#ifndef QAT_CLI_DISPLAY_HPP
#define QAT_CLI_DISPLAY_HPP

#include "../utils/types.hpp"
#include "./color.hpp"
#include "./version.hpp"
#include <iostream>
#include <string>

namespace qat {
namespace CLI {
namespace display {
/**
 * @brief Displays version of the compiler (Short form)
 *
 */
void version();
/**
 * @brief Displays details about the compiler, like version build info,
 * website...
 *
 * @param buildCommit
 */
void about(std::string buildCommit);

/**
 * @brief Displays the build info
 *
 * @param buildCommit
 */
void build_info(std::string buildCommit);

/**
 * @brief Displays detailed information on how to use the CLI of the compiler.
 *
 */
void help();

/**
 * @brief Displays websites related to the project
 *
 */
void websites();

/**
 * @brief Displays information about potential errors that are shown while using
 * the language in a wrong manner
 *
 */
void errors();

} // namespace display
} // namespace CLI
} // namespace qat

#endif