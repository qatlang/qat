#ifndef QAT_UTILITIES_DIRECTORY_CONTENTS_HPP
#define QAT_UTILITIES_DIRECTORY_CONTENTS_HPP

#include <experimental/filesystem>
#include <vector>

namespace fsexp = std::experimental::filesystem;
namespace qat {
namespace utilities {
std::vector<fsexp::path> directoryContents(fsexp::path path, bool recursive);
} // namespace utilities
} // namespace qat

#endif