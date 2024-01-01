#include "./visibility.hpp"
#include "../IR/qat_module.hpp"
#include "../IR/types/qat_type.hpp"

namespace qat {

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

AccessInfo::AccessInfo(IR::QatModule* _module, Maybe<IR::QatType*> _type) : module(_module), type(_type) {}

AccessInfo AccessInfo::GetPrivileged() {
  AccessInfo result   = AccessInfo(nullptr, None);
  result.isPrivileged = true;
  return result;
}

bool AccessInfo::isPrivilegedAccess() const { return isPrivileged; }

bool AccessInfo::hasType() const { return type.has_value() && (type.value() != nullptr); }

IR::QatModule* AccessInfo::getModule() const { return module; }

IR::QatType* AccessInfo::getType() const { return type.value_or(nullptr); }

bool Visibility::isAccessible(const VisibilityInfo& visibility, Maybe<AccessInfo> req_info) {
  if (req_info.has_value() && req_info->isPrivilegedAccess()) {
    return true;
  }
  switch (visibility.kind) {
    case VisibilityKind::box:
    case VisibilityKind::file:
    case VisibilityKind::folder:
    case VisibilityKind::lib: {
      SHOW("VisibilityInfo: Module")
      return req_info.has_value() && ((visibility.moduleVal->getID() == req_info->getModule()->getID()) ||
                                      visibility.moduleVal->isParentModuleOf(req_info->getModule()));
    }
    case VisibilityKind::pub: {
      SHOW("VisibilityInfo: PUB")
      return true;
    }
    case VisibilityKind::type: {
      SHOW("VisibilityInfo: TYPE")
      if (req_info.has_value() && req_info->hasType()) {
        return req_info->getType()->isSame(visibility.typePtr);
      }
      return false;
    }
    case VisibilityKind::parent: {
      SHOW("VisibilityInfo: PARENT")
      if (!req_info.has_value()) {
        return false;
      }
      if (visibility.typePtr && req_info->hasType()) {
        return visibility.typePtr->isSame(req_info->getType());
      } else if (visibility.moduleVal && req_info->getModule()) {
        return (visibility.moduleVal->getID() == req_info->getModule()->getID()) ||
               visibility.moduleVal->isParentModuleOf(req_info->getModule());
      }
      return false;
    }
  }
}

bool VisibilityInfo::isAccessible(Maybe<AccessInfo> reqInfo) const { return Visibility::isAccessible(*this, reqInfo); }

bool VisibilityInfo::operator==(const VisibilityInfo& other) const {
  return (kind == other.kind) && (typePtr == other.typePtr) && (moduleVal == other.moduleVal);
}

VisibilityInfo::operator Json() const {
  return Json()
      ._("nature", Visibility::getValue(kind))
      ._("hasModule", moduleVal != nullptr)
      ._("moduleID", moduleVal ? moduleVal->getID() : "")
      ._("hasType", typePtr != nullptr)
      ._("typeID", typePtr ? typePtr->getID() : "");
}

VisibilityInfo::operator JsonValue() const { return (Json)(*this); }

VisibilityInfo::VisibilityInfo(const VisibilityInfo& other)
    : kind(other.kind), moduleVal(other.moduleVal), typePtr(other.typePtr) {}

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

} // namespace qat