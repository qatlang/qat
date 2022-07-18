#ifndef QAT_UTILS_FILE_PLACEMENT_HPP
#define QAT_UTILS_FILE_PLACEMENT_HPP

#include <cstdint>
#include <filesystem>
#include <nuo/json.hpp>

namespace fs = std::filesystem;

namespace qat::utils {

// Position indicates a line and character number in a file
struct FilePos {
  FilePos(nuo::Json);

  FilePos(uint64_t line, uint64_t character);

  // Number of the line of this position. This will never be negative
  uint64_t line;
  // Number of the character of this position. This will never be negative
  uint64_t character;

  operator nuo::JsonValue() const;

  operator nuo::Json() const;
};

/** FileRange represents a particular range in a file */
class FileRange {
public:
  /**
   *  FileRange represents a particular range in a file
   *
   * @param file The file this placement belongs to
   * @param start The beginning position of the placement
   * @param end The ending position of the placement
   */
  FileRange(fs::path _file, FilePos _start, FilePos _end);

  FileRange(nuo::Json json);

  /**
   *  FileRange represents a partivular range in a file
   *
   * This creates a FileRange from two other FileRanges. The beginning
   * of the first one will be the beginning of the new one. The end of the
   * second one will be the end of the new one, unless the files of the provided
   * placements are different
   *
   * @param first The placement from which the start of the new placement
   * is obtained
   * @param second The placement from which the end of the new placement
   * is obtained. This argument is ignored if the files of both FileRanges
   * don't match
   */
  FileRange(FileRange first, FileRange second);

  /** Path to the corresponding file */
  fs::path file;

  /** Starting position of the range */
  FilePos start;

  /** Ending position of the range */
  FilePos end;

  operator nuo::Json() const;

  operator nuo::JsonValue() const;
};

} // namespace qat::utils

#endif