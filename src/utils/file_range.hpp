#ifndef QAT_UTILS_FILE_PLACEMENT_HPP
#define QAT_UTILS_FILE_PLACEMENT_HPP

#include "./json.hpp"
#include <cstdint>
#include <filesystem>

namespace qat {

struct FilePos {
	explicit FilePos(Json json);
	FilePos(u64 line, u64 byteOffset);

	u64 line;
	u64 byteOffset;

	operator JsonValue() const;

	operator Json() const;
};

std::ostream& operator<<(std::ostream& os, FilePos const& pos);

class FileRange {
  public:
	FileRange(fs::path _filePath);

	FileRange(fs::path _file, FilePos _start, FilePos _end);

	FileRange(Json json);

	FileRange(const FileRange& first, const FileRange& second);

	fs::path file;
	FilePos start;
	FilePos end;

	useit FileRange spanTo(FileRange const& other) const;
	useit FileRange trimTo(FilePos othStart) const;
	useit String    start_to_string() const;
	useit bool      is_before(FileRange another) const;

	operator Json() const;
	operator JsonValue() const;
	useit Json to_json() const;
	useit JsonValue to_json_value() const;
};

std::ostream& operator<<(std::ostream& os, FileRange const& range);

} // namespace qat

#endif
