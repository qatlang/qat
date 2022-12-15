#include "./visibility.hpp"

namespace qat::utils {

#define VISIB_KIND  "visibility_kind"
#define VISIB_VALUE "visibility_value"

JsonValue kindToJsonValue(VisibilityKind kind) {
  switch (kind) {
    case VisibilityKind::type:
      return "type";
    case VisibilityKind::pub:
      return "pub";
    case VisibilityKind::lib:
      return "lib";
    case VisibilityKind::file:
      return "file";
    case VisibilityKind::folder:
      return "folder";
    case VisibilityKind::parent:
      return "parent";
    case VisibilityKind::box:
      return "box";
  }
}

RequesterInfo::RequesterInfo(Maybe<String> _lib, Maybe<String> _box, String _file, Maybe<String> _type)
    : lib(std::move(_lib)), box(std::move(_box)), file(std::move(_file)), type(std::move(_type)) {}

bool RequesterInfo::hasLib() const { return lib.has_value(); }

bool RequesterInfo::hasBox() const { return box.has_value(); }

bool RequesterInfo::hasType() const { return type.has_value(); }

String RequesterInfo::getLib() const { return lib.value_or(""); }

String RequesterInfo::getBox() const { return box.value_or(""); }

String RequesterInfo::getType() const { return type.value_or(""); }

String RequesterInfo::getFile() const { return file; }

bool Visibility::isAccessible(const VisibilityInfo& visibility, const RequesterInfo& req_info) {
  switch (visibility.kind) {
    case VisibilityKind::box: {
      if (req_info.hasBox()) {
        return (req_info.getBox().find(visibility.value) == 0);
      }
      return false;
    }
    case VisibilityKind::file: {
      return (visibility.value == req_info.getFile());
    }
    case VisibilityKind::folder: {
      return (req_info.getFile().find(visibility.value) == 0);
    }
    case VisibilityKind::lib: {
      if (req_info.hasLib()) {
        return (req_info.getLib().find(visibility.value) == 0);
      }
      return false;
    }
    case VisibilityKind::pub: {
      return true;
    }
    case VisibilityKind::type: {
      if (req_info.hasType()) {
        return (req_info.getType().find(visibility.value) == 0);
      }
      return false;
    }
    case VisibilityKind::parent:
      return false;
  }
}

bool VisibilityInfo::isAccessible(const RequesterInfo& reqInfo) const {
  return Visibility::isAccessible(*this, reqInfo);
}

bool VisibilityInfo::operator==(const VisibilityInfo& other) const {
  return (kind == other.kind) && (value == other.value);
}

VisibilityInfo::operator Json() const { return Json()._("nature", Visibility::getValue(kind))._("value", value); }

VisibilityInfo::operator JsonValue() const { return (Json)(*this); }

VisibilityInfo::VisibilityInfo(const VisibilityInfo& other) = default;

const std::map<VisibilityKind, String> Visibility::kind_value_map = {
    {VisibilityKind::pub, "public"},   {VisibilityKind::type, "type"}, {VisibilityKind::lib, "library"},
    {VisibilityKind::file, "file"},    {VisibilityKind::box, "box"},   {VisibilityKind::parent, "parent"},
    {VisibilityKind::folder, "folder"}};

const std::map<String, VisibilityKind> Visibility::value_kind_map = {
    {"public", VisibilityKind::pub},   {"type", VisibilityKind::type}, {"library", VisibilityKind::lib},
    {"file", VisibilityKind::file},    {"box", VisibilityKind::box},   {"parent", VisibilityKind::parent},
    {"folder", VisibilityKind::folder}};

String Visibility::getValue(VisibilityKind kind) { return kind_value_map.find(kind)->second; }

VisibilityKind Visibility::getKind(const String& value) { return value_kind_map.find(value)->second; }

} // namespace qat::utils