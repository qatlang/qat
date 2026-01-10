#ifndef QAT_CLI_ERROR_HPP
#define QAT_CLI_ERROR_HPP

#include <helpers/files.hpp>
#include <helpers/maybe.hpp>
#include <helpers/string.hpp>

namespace qat::cli {

void Error(String const& message, Maybe<FilePath> path);
void Warning(String const& message, Maybe<FilePath> path);

} // namespace qat::cli

#endif
