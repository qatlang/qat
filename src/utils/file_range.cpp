#include "./file_range.hpp"
#include "./json.hpp"
#include "./qat_region.hpp"

#include <filesystem>

namespace qat {

FilePos::FilePos(Json json)
    : line(std::stoul(json["line"].asString())), byteOffset(std::stoul(json["byteOffset"].asString())) {}

FilePos::FilePos(u64 _line, u64 _byte) : line(_line), byteOffset(_byte) {}

FilePos::operator JsonValue() const { return (Json)(*this); }

FilePos::operator Json() const { return Json()._("line", line)._("byteOffset", byteOffset); }

std::ostream& operator<<(std::ostream& os, FilePos const& pos) { return os << pos.line << ":" << pos.byteOffset; }

FileRangePtr FileRange::null = nullptr;

FileRangePtr FileRange::from_path(fs::path _filePath) {
	return std::construct_at(OwnNormal(FileRange), std::move(_filePath), FilePos{0u, 0u}, FilePos{0u, 0u});
}

FileRangePtr FileRange::from(fs::path _file, FilePos _start, FilePos _end) {
	return std::construct_at(OwnNormal(FileRange), std::move(_file), _start, _end);
}

FileRange* FileRange::var_from(fs::path _file, FilePos _start, FilePos _end) {
	return std::construct_at(OwnNormal(FileRange), std::move(_file), _start, _end);
}

FileRangePtr FileRange::merge(FileRange const* first, FileRange const* second) {
	return std::construct_at(OwnNormal(FileRange), first->file, first->start, second->end);
}

FileRangePtr FileRange::from_json(Json json) {
	return std::construct_at(OwnNormal(FileRange), fs::path(json["file"].asString()), FilePos(json["start"].asJson()),
	                         FilePos(json["end"].asJson()));
}

FileRangePtr FileRange::spanTo(FileRangePtr other) const { return FileRange::merge(this, other); }

FileRangePtr FileRange::trimTo(FilePos othStart) const { return FileRange::from(file, start, othStart); }

String FileRange::start_to_string() const {
	return file.string() + ":" + std::to_string(start.line) + ":" + std::to_string(start.byteOffset);
}

bool FileRange::is_before(FileRangePtr another) const {
	return fs::equivalent(file, another->file) &&
	       ((end.line < another->start.line) ||
	        ((end.line == another->start.line) && (end.byteOffset < another->start.byteOffset)));
}

Json FileRange::to_json() const { return Json()._("path", file.string())._("start", start)._("end", end); }

JsonValue FileRange::to_json_value() const { return to_json(); }

String FileRange::to_string() const { return file.string() + ":" + (String)start + " - " + (String)end; }

} // namespace qat
