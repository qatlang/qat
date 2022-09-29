#ifndef QAT_UTILS_FILE_PLACEMENT_HPP
#define QAT_UTILS_FILE_PLACEMENT_HPP

#include "./json.hpp"
#include <cstdint>
#include <filesystem>

namespace qat::utils {

// Position indicates a line and character number in a file
struct FilePos {
  FilePos(Json);

  FilePos(uint64_t line, uint64_t character);

  // Number of the line of this position. This will never be negative
  uint64_t line;
  // Number of the character of this position. This will never be negative
  uint64_t character;

  operator JsonValue() const;

  operator Json() const;
};

/** FileRange represents a particular range in a file */
class FileRange {
public:
  /**
   *  FileRange represents a particular range in a file
   *
   * @param file The file this fileRange belongs to
   * @param start The beginning position of the fileRange
   * @param end The ending position of the fileRange
   */
  FileRange(fs::path _file, FilePos _start, FilePos _end);

  FileRange(Json json);

  /**
   *  FileRange represents a partivular range in a file
   *
   * This creates a FileRange from two other FileRanges. The beginning
   * of the first one will be the beginning of the new one. The end of the
   * second one will be the end of the new one, unless the files of the provided
   * fileRanges are different
   *
   * @param first The fileRange from which the start of the new fileRange
   * is obtained
   * @param second The fileRange from which the end of the new fileRange
   * is obtained. This argument is ignored if the files of both FileRanges
   * don't match
   */
  FileRange(const FileRange &first, const FileRange &second);

  /** Path to the corresponding file */
  fs::path file;

  /** Starting position of the range */
  FilePos start;

  /** Ending position of the range */
  FilePos end;

  operator Json() const;

  operator JsonValue() const;
};

} // namespace qat::utils

#endif