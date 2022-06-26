#ifndef QAT_UTILS_FILE_PLACEMENT_HPP
#define QAT_UTILS_FILE_PLACEMENT_HPP

#include <filesystem>

namespace fs = std::filesystem;

namespace qat {
namespace utils {
/** Position indicates a line and character number in a file */
struct Position {
  /** Number of the line of this position. This will never be negative */
  unsigned long long line;

  /** Number of the character of this position. This will never be negative */
  unsigned long long character;
};

/** FilePlacement represents a particular range in a file */
class FilePlacement {
public:
  /**
   *  FilePlacement represents a particular range in a file
   *
   * @param file The file this placement belongs to
   * @param start The beginning position of the placement
   * @param end The ending position of the placement
   */
  FilePlacement(fs::path _file, Position _start, Position _end);

  /**
   *  FilePlacement represents a partivular range in a file
   *
   * This creates a FilePlacement from two other FilePlacements. The beginning
   * of the first one will be the beginning of the new one. The end of the
   * second one will be the end of the new one, unless the files of the provided
   * placements are different
   *
   * @param first The placement from which the start of the new placement
   * is obtained
   * @param second The placement from which the end of the new placement
   * is obtained. This argument is ignored if the files of both FilePlacements
   * don't match
   */
  FilePlacement(FilePlacement first, FilePlacement second);

  /** Path to the corresponding file */
  fs::path file;

  /** Starting position of the range */
  Position start;

  /** Ending position of the range */
  Position end;
};
} // namespace utils
} // namespace qat

#endif