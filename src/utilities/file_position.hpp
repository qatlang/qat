#ifndef QAT_UTILITIES_FILE_POSITION_HPP
#define QAT_UTILITIES_FILE_POSITION_HPP

#include <experimental/filesystem>

namespace fsexp = std::experimental::filesystem;

namespace qat {
namespace utilities {
class FilePosition {
public:
  FilePosition(fsexp::path _file, unsigned long long _line,
               unsigned long long _character)
      : file(_file), line(_line), character(_character) {}

  fsexp::path file;
  unsigned long long line;
  unsigned long long character;
};
} // namespace utilities
} // namespace qat

#endif