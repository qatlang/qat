#include "./file_placement.hpp"
#include <nuo/json.hpp>
namespace qat {
namespace utils {

Position::Position(uint64_t _line, uint64_t _character)
    : line(_line), character(_character) {}

Position::Position(nuo::Json json)
    : line(json["line"].asInt()), character(json["char"].asInt()) {}

Position::operator nuo::JsonValue() const {
  return nuo::JsonValue((nuo::Json)(*this));
}

Position::operator nuo::Json() const {
  return nuo::Json()._("line", line)._("char", character);
}

FilePlacement::FilePlacement(fs::path _file, Position _start, Position _end)
    : file(_file), start(_start), end(_end) {}

FilePlacement::FilePlacement(FilePlacement first, FilePlacement second)
    : file(first.file), start(first.start),
      end((first.file == second.file) ? second.end : first.end) {}

FilePlacement::FilePlacement(nuo::Json json)
    : file(json["file"].asString()), start(json["start"].asJson()),
      end(json["end"].asJson()) {}

FilePlacement::operator nuo::Json() const {
  return nuo::Json()._("path", file.string())._("start", start)._("end", end);
}

FilePlacement::operator nuo::JsonValue() const {
  return nuo::JsonValue((nuo::Json)(*this));
}

} // namespace utils
} // namespace qat