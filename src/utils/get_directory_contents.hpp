#ifndef QAT_UTILS_GET_DIRECTORY_CONTENTS_HPP
#define QAT_UTILS_GET_DIRECTORY_CONTENTS_HPP

#include <filesystem>
#include <vector>

namespace fs = std::filesystem;
namespace qat::utils {

std::vector<fs::path> get_directory_contents(fs::path path, bool recursive);

} // namespace qat::utils

#endif