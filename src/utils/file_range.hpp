#ifndef QAT_UTILS_FILE_PLACEMENT_HPP
#define QAT_UTILS_FILE_PLACEMENT_HPP

#include "./helpers.hpp"

#include <boost/json.hpp>
#include <filesystem>

namespace qat {

struct FilePos {
	FilePos(u64 line, u64 byteOffset);

	u64 line;
	u64 byteOffset;

	boost::json::object to_json() const { return {{"line", line}, {"byteOffset", byteOffset}}; }

	String to_string() const { return std::to_string(line) + ":" + std::to_string(byteOffset); }
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

	static FileRangePtr from_path(String* _filePath);

	static FileRangePtr from(String* _file, FilePos _start, FilePos _end);

	static FileRange* var_from(String* _file, FilePos _start, FilePos _end);

	static FileRangePtr merge(FileRangePtr first, FileRangePtr second);

	FileRangePtr spanTo(FileRangePtr other) const;

	FileRangePtr trimTo(FilePos othStart) const;

	String start_to_string() const;

	bool is_before(FileRangePtr another) const;

	boost::json::object to_json() const {
		return {{"path", *file}, {"start", start.to_json()}, {"end", end.to_json()}};
	}

	String to_string() const;
};

std::ostream& operator<<(std::ostream& os, FileRangePtr) = delete;
std::ostream& operator<<(std::ostream& os, FileRange*)   = delete;

} // namespace qat

#endif
