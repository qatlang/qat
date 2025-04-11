#include "./file_range.hpp"
#include "./json.hpp"
#include <filesystem>

namespace qat {

FilePos::FilePos(u64 _line, u64 _byte) : line(_line), byteOffset(_byte) {}

FilePos::operator JsonValue() const { return (Json)(*this); }

FilePos::operator Json() const { return Json()._("line", line)._("byteOffset", byteOffset); }

std::ostream& operator<<(std::ostream& os, FilePos const& pos) { return os << pos.line << ":" << pos.byteOffset; }

FileRange::FileRange(fs::path _filePath) : file(std::move(_filePath)), start({0u, 0u}), end({0u, 0u}) {}

FileRange::FileRange(fs::path _file, FilePos _start, FilePos _end) : file(std::move(_file)), start(_start), end(_end) {}

FileRange::FileRange(const FileRange& first, const FileRange& second)
    : file(first.file), start(first.start), end((first.file == second.file) ? second.end : first.end) {}

FileRange::FileRange(Json json)
    : file(json["file"].asString()), start(json["start"].asJson()), end(json["end"].asJson()) {}

FileRange FileRange::spanTo(FileRange const& other) const { return FileRange{*this, other}; }

FileRange FileRange::trimTo(FilePos othStart) const { return FileRange(file, start, othStart); }

String FileRange::start_to_string() const {
	return file.string() + ":" + std::to_string(start.line) + ":" + std::to_string(start.byteOffset);
}

bool FileRange::is_before(FileRange another) const {
	return std::filesystem::equivalent(file, another.file) &&
	       ((end.line < another.start.line) ||
	        ((end.line == another.start.line) && (end.byteOffset < another.start.byteOffset)));
}

FileRange::operator Json() const { return Json()._("path", file.string())._("start", start)._("end", end); }

FileRange::operator JsonValue() const { return (Json)(*this); }

std::ostream& operator<<(std::ostream& os, FileRange const& range) {
	return os << range.file.string() << ":" << range.start << " - " << range.end;
}

} // namespace qat
