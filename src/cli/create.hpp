#ifndef QAT_CLI_CREATE_HPP
#define QAT_CLI_CREATE_HPP

#include <helpers/files.hpp>
#include <helpers/maybe.hpp>
#include <helpers/string.hpp>

namespace qat::cli {

void create_project(String name, FilePath path, bool isLib, Maybe<String> vcs);

}

#endif
