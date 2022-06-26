#include "./file_placement.hpp"
namespace qat {
namespace utils {

FilePlacement::FilePlacement(fs::path _file, Position _start, Position _end)
    : file(_file), start(_start), end(_end) {}

FilePlacement::FilePlacement(FilePlacement first, FilePlacement second)
    : file(first.file), start(first.start),
      end((first.file == second.file) ? second.end : first.end) {}

} // namespace utils
} // namespace qat