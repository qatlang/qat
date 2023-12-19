#include "./file_range.hpp"
#include "../show.hpp"
#include "./json.hpp"
#include <filesystem>
namespace qat {

FilePos::FilePos(uint64_t _line, uint64_t _character) : line(_line), character(_character) {}

FilePos::FilePos(Json json) : line(json["line"].asInt()), character(json["char"].asInt()) {}

FilePos::operator JsonValue() const { return (Json)(*this); }

FilePos::operator Json() const { return Json()._("line", line)._("char", character); }

std::ostream& operator<<(std::ostream& os, FilePos const& pos) { return os << pos.line << ":" << pos.character; }

FileRange::FileRange(fs::path _filePath) : file(std::move(_filePath)), start({0u, 0u}), end({0u, 0u}) {}

FileRange::FileRange(fs::path _file, FilePos _start, FilePos _end) : file(std::move(_file)), start(_start), end(_end) {}

FileRange::FileRange(const FileRange& first, const FileRange& second)
    : file(first.file), start(first.start), end((first.file == second.file) ? second.end : first.end) {}

FileRange::FileRange(Json json)
    : file(json["file"].asString()), start(json["start"].asJson()), end(json["end"].asJson()) {}

FileRange FileRange::spanTo(FileRange const& other) const { return FileRange{*this, other}; }

FileRange FileRange::trimTo(FilePos othStart) const { return FileRange(file, start, othStart); }

String FileRange::startToString() const {
  return file.string() + ":" + std::to_string(start.line) + ":" + std::to_string(start.character);
}

FileRange::operator Json() const { return Json()._("path", file.string())._("start", start)._("end", end); }

FileRange::operator JsonValue() const { return (Json)(*this); }

std::ostream& operator<<(std::ostream& os, FileRange const& range) {
  return os << range.file.string() << ":" << range.start << " - " << range.end;
}

} // namespace qat