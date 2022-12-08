#ifndef QAT_CLI_ERROR_HPP
#define QAT_CLI_ERROR_HPP

#include "../utils/helpers.hpp"
#include "./color.hpp"
#include <iostream>

namespace qat::cli {

void Error(const String& message, Maybe<fs::path> path);
void Warning(const String& message, Maybe<fs::path> path);

} // namespace qat::cli

#endif