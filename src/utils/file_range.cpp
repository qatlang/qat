#include "./file_range.hpp"
#include "../show.hpp"
#include "./json.hpp"
#include <filesystem>
namespace qat {

FilePos::FilePos(uint64_t _line, uint64_t _character) : line(_line), character(_character) {}

FilePos::FilePos(Json json) : line(json["line"].asInt()), character(json["char"].asInt()) {}

FilePos::operator JsonValue() const { return (Json)(*this); }

FilePos::operator Json() const {
  return Json()._("line", (unsigned long long)line)._("char", (unsigned long long)character);
}

std::ostream& operator<<(std::ostream& os, FilePos const& pos) { return os << pos.line << ":" << pos.character; }

FileRange::FileRange(fs::path _filePath) : file(std::move(_filePath)), start({0u, 0u}), end({0u, 0u}) {}

FileRange::FileRange(fs::path _file, FilePos _start, FilePos _end) : file(std::move(_file)), start(_start), end(_end) {}

FileRange::FileRange(const FileRange& first, const FileRange& second)
    : file(first.file), start(first.start), end((first.file == second.file) ? second.end : first.end) {}

FileRange::FileRange(Json json)
    : file(json["file"].asString()), start(json["start"].asJson()), end(json["end"].asJson()) {}

FileRange::operator Json() const {
  String absPath;
  try {
    absPath = fs::absolute(file).lexically_normal().string();
  } catch (std::exception& e) {
    SHOW("Exception while making path absolute:")
    SHOW("Path: " << file)
    SHOW("Exception is: " << e.what())
    absPath = file.string();
  }
  return Json()._("path", absPath)._("start", start)._("end", end);
}

FileRange::operator JsonValue() const { return (Json)(*this); }

std::ostream& operator<<(std::ostream& os, FileRange const& range) {
  return os << "FileRange { " << range.file.string() << ":" << range.start << "-" << range.end << " }";
}

} // namespace qat