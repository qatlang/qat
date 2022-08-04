#ifndef QAT_UTILS_GET_DIRECTORY_CONTENTS_HPP
#define QAT_UTILS_GET_DIRECTORY_CONTENTS_HPP

#include "./helpers.hpp"

namespace qat::utils {

Vec<fs::path> get_directory_contents(fs::path path, bool recursive);

} // namespace qat::utils

#endif