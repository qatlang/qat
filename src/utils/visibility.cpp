#include "./visibility.hpp"

namespace qat {
namespace utils {

#define VISIB_KIND "visibility_kind"
#define VISIB_VALUE "visibility_value"

bool Visibility::isAccessible(VisibilityInfo visibility,
                              const RequesterInfo req_info) {
  switch (visibility.kind) {
  case VisibilityKind::box: {
    if (req_info.box.has_value()) {
      return (req_info.box.value().find(visibility.value) == 0);
    } else {
      return false;
    }
  }
  case VisibilityKind::file: {
    return (visibility.value == req_info.file);
  }
  case VisibilityKind::folder: {
    return (req_info.file.find(visibility.value) == 0);
  }
  case VisibilityKind::lib: {
    if (req_info.lib.has_value()) {
      return (req_info.lib.value().find(visibility.value) == 0);
    } else {
      return false;
    }
  }
  case VisibilityKind::pub: {
    return true;
  }
  case VisibilityKind::type: {
    if (req_info.type.has_value()) {
      return (req_info.type.value().find(visibility.value) == 0);
    } else {
      return false;
    }
  }
  }
}

bool VisibilityInfo::isAccessible(const RequesterInfo &reqInfo) const {
  return Visibility::isAccessible(*this, reqInfo);
}

bool VisibilityInfo::operator==(VisibilityInfo other) const {
  return (kind == other.kind) && (value == other.value);
}

VisibilityInfo::operator nuo::Json() const {
  return nuo::Json()._("kind", Visibility::getValue(kind))._("value", value);
}

VisibilityInfo::operator nuo::JsonValue() const {
  return nuo::JsonValue((nuo::Json)(*this));
}

VisibilityInfo::VisibilityInfo(const VisibilityInfo &other)
    : kind(other.kind), value(other.value) {}

const std::map<VisibilityKind, std::string> Visibility::kind_value_map = {
    {VisibilityKind::pub, "public"},
    {VisibilityKind::type, "type"},
    {VisibilityKind::lib, "library"},
    {VisibilityKind::file, "file"},
    {VisibilityKind::box, "box"}};

const std::map<std::string, VisibilityKind> Visibility::value_kind_map = {
    {"public", VisibilityKind::pub},
    {"type", VisibilityKind::type},
    {"library", VisibilityKind::lib},
    {"file", VisibilityKind::file},
    {"box", VisibilityKind::box}};

std::string Visibility::getValue(VisibilityKind kind) {
  return kind_value_map.find(kind)->second;
}

VisibilityKind Visibility::getKind(std::string value) {
  return value_kind_map.find(value)->second;
}

} // namespace utils
} // namespace qat