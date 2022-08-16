#include "./file_range.hpp"
#include <nuo/json.hpp>
namespace qat::utils {

FilePos::FilePos(uint64_t _line, uint64_t _character)
    : line(_line), character(_character) {}

FilePos::FilePos(nuo::Json json)
    : line(json["line"].asInt()), character(json["char"].asInt()) {}

FilePos::operator nuo::JsonValue() const { return (nuo::Json)(*this); }

FilePos::operator nuo::Json() const {
  return nuo::Json()
      ._("line", (unsigned long long)line)
      ._("char", (unsigned long long)character);
}

FileRange::FileRange(fs::path _file, FilePos _start, FilePos _end)
    : file(std::move(_file)), start(_start), end(_end) {}

FileRange::FileRange(const FileRange &first, const FileRange &second)
    : file(first.file), start(first.start),
      end((first.file == second.file) ? second.end : first.end) {}

FileRange::FileRange(nuo::Json json)
    : file(json["file"].asString()), start(json["start"].asJson()),
      end(json["end"].asJson()) {}

FileRange::operator nuo::Json() const {
  return nuo::Json()._("path", file.string())._("start", start)._("end", end);
}

FileRange::operator nuo::JsonValue() const { return (nuo::Json)(*this); }

} // namespace qat::utils