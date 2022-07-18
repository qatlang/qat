#ifndef QAT_CLI_ERROR_HPP
#define QAT_CLI_ERROR_HPP

#include "./color.hpp"
#include <filesystem>
#include <iostream>
#include <string>

namespace qat::cli {

void throw_error(std::string message, std::filesystem::path path);

} // namespace qat::cli

#endif