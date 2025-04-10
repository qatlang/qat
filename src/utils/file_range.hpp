#ifndef QAT_UTILS_FILE_PLACEMENT_HPP
#define QAT_UTILS_FILE_PLACEMENT_HPP

#include "./json.hpp"
#include <cstdint>
#include <filesystem>

namespace qat {

struct FilePos {
	FilePos(Json);

	FilePos(u64 line, u64 byteOffset);

	u64 line;
	u64 byteOffset;

	operator JsonValue() const;

	operator Json() const;
};

std::ostream& operator<<(std::ostream& os, FilePos const& pos);

/** FileRange represents a particular range in a file */
class FileRange {
  public:
	FileRange(fs::path _filePath);

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
	 *  FileRange represents a particular range in a file
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
	FileRange(const FileRange& first, const FileRange& second);

	/** Path to the corresponding file */
	fs::path file;

	/** Starting position of the range */
	FilePos start;

	/** Ending position of the range */
	FilePos end;

	useit FileRange spanTo(FileRange const& other) const;
	useit FileRange trimTo(FilePos othStart) const;
	useit String    start_to_string() const;
	useit bool      is_before(FileRange another) const;

	operator Json() const;

	operator JsonValue() const;
};

std::ostream& operator<<(std::ostream& os, FileRange const& range);

} // namespace qat

#endif
