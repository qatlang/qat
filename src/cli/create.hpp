#ifndef QAT_CLI_CREATE_HPP
#define QAT_CLI_CREATE_HPP

#include "../utils/helpers.hpp"

namespace qat::cli {

void create_project(String name, fs::path path, bool isLib, Maybe<String> vcs);

}

#endif