#ifndef QAT_CLI_ERROR_HPP
#define QAT_CLI_ERROR_HPP

#include "../utils/helpers.hpp"
#include "./color.hpp"
#include <iostream>

namespace qat::cli {

void throw_error(String message, fs::path path);

} // namespace qat::cli

#endif