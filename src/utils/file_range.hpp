#ifndef QAT_UTILS_FILE_PLACEMENT_HPP
#define QAT_UTILS_FILE_PLACEMENT_HPP

#include "./json.hpp"

#include <filesystem>

namespace qat {

struct FilePos {
	explicit FilePos(Json json);
	FilePos(u64 line, u64 byteOffset);

	u64 line;
	u64 byteOffset;

	operator JsonValue() const;
	operator Json() const;

	operator String() const { return std::to_string(line) + ":" + std::to_string(byteOffset); }
};

std::ostream& operator<<(std::ostream& os, FilePos const& pos);

class FileRange;
using FileRangePtr = FileRange const*;

class FileRange {
  public:
	String* file;
	FilePos start;
	FilePos end;

	FileRange(String* _path, FilePos _start, FilePos _end) : file(_path), start(_start), end(_end) {}

	FileRange(FileRange const&) = delete;
	FileRange(FileRange&&)      = delete;

	static FileRangePtr null;

	useit static FileRangePtr from_path(String* _filePath);

	useit static FileRangePtr from(String* _file, FilePos _start, FilePos _end);

	useit static FileRange* var_from(String* _file, FilePos _start, FilePos _end);

	useit static FileRangePtr merge(FileRangePtr first, FileRangePtr second);

	useit FileRangePtr spanTo(FileRangePtr other) const;
	useit FileRangePtr trimTo(FilePos othStart) const;
	useit String       start_to_string() const;
	useit bool         is_before(FileRangePtr another) const;
	useit String       to_string() const;
	useit Json         to_json() const;
	useit JsonValue    to_json_value() const;
};

std::ostream& operator<<(std::ostream& os, FileRangePtr) = delete;
std::ostream& operator<<(std::ostream& os, FileRange*)   = delete;

} // namespace qat

#endif
