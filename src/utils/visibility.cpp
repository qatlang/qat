#include "./visibility.hpp"
#include "../IR/qat_module.hpp"
#include "../IR/skill.hpp"
#include "../IR/types/qat_type.hpp"

namespace qat {

#define VISIB_KIND  "visibility_kind"
#define VISIB_VALUE "visibility_value"

String kind_to_string(VisibilityKind kind) {
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
		case VisibilityKind::skill:
			return "skill";
	}
}

AccessInfo::AccessInfo(ir::Mod* _module, Maybe<ir::Type*> _type, Maybe<ir::DoneSkill*> _skill)
    : module(_module), type(_type), skill(_skill) {}

AccessInfo AccessInfo::get_privileged() {
	AccessInfo result   = AccessInfo(nullptr, None, None);
	result.isPrivileged = true;
	return result;
}

bool AccessInfo::is_privileged_access() const { return isPrivileged; }

bool Visibility::is_accessible(const VisibilityInfo& visibility, Maybe<AccessInfo> reqInfo) {
	if (reqInfo.has_value() && reqInfo->is_privileged_access()) {
		return true;
	}
	switch (visibility.kind) {
		case VisibilityKind::file:
		case VisibilityKind::folder:
		case VisibilityKind::lib: {
			return reqInfo.has_value() && ((visibility.moduleVal->get_id() == reqInfo->get_module()->get_id()) ||
			                               visibility.moduleVal->is_parent_mod_of(reqInfo->get_module()));
		}
		case VisibilityKind::pub: {
			return true;
		}
		case VisibilityKind::type: {
			if (reqInfo.has_value() && reqInfo->has_type()) {
				return reqInfo->get_type()->is_same(visibility.typePtr);
			}
			return false;
		}
		case VisibilityKind::skill: {
			if (reqInfo.has_value() && reqInfo->has_skill()) {
				return reqInfo->get_skill()->get_ir_type()->is_same(visibility.typePtr);
			}
		}
		case VisibilityKind::parent: {
			if (not reqInfo.has_value()) {
				return false;
			}
			if (visibility.typePtr && reqInfo->has_type()) {
				return visibility.typePtr->is_same(reqInfo->get_type());
			} else if (visibility.typePtr && reqInfo->has_skill()) {
				return reqInfo->get_skill()->get_ir_type()->is_same(visibility.typePtr);
			} else if (visibility.moduleVal && reqInfo->get_module()) {
				return (visibility.moduleVal->get_id() == reqInfo->get_module()->get_id()) ||
				       visibility.moduleVal->is_parent_mod_of(reqInfo->get_module());
			}
			return false;
		}
	}
}

bool VisibilityInfo::is_accessible(Maybe<AccessInfo> reqInfo) const {
	return Visibility::is_accessible(*this, reqInfo);
}

bool VisibilityInfo::operator==(const VisibilityInfo& other) const {
	return (kind == other.kind) && (typePtr == other.typePtr) && (moduleVal == other.moduleVal);
}

VisibilityInfo::operator Json() const {
	return Json()
	    ._("nature", Visibility::getValue(kind))
	    ._("hasModule", moduleVal != nullptr)
	    ._("moduleID", moduleVal ? moduleVal->get_id() : JsonValue())
	    ._("hasType", typePtr != nullptr)
	    ._("typeID", typePtr ? typePtr->get_id() : JsonValue());
}

VisibilityInfo::operator JsonValue() const { return (Json)(*this); }

VisibilityInfo::VisibilityInfo(const VisibilityInfo& other)
    : kind(other.kind), moduleVal(other.moduleVal), typePtr(other.typePtr) {}

const std::map<VisibilityKind, String> Visibility::kindValueMap = {
    {VisibilityKind::pub, "public"}, {VisibilityKind::type, "type"},     {VisibilityKind::lib, "library"},
    {VisibilityKind::file, "file"},  {VisibilityKind::parent, "parent"}, {VisibilityKind::folder, "folder"}};

const std::map<String, VisibilityKind> Visibility::valueKindMap = {
    {"public", VisibilityKind::pub}, {"type", VisibilityKind::type},     {"library", VisibilityKind::lib},
    {"file", VisibilityKind::file},  {"parent", VisibilityKind::parent}, {"folder", VisibilityKind::folder}};

String Visibility::getValue(VisibilityKind kind) { return kindValueMap.find(kind)->second; }

VisibilityKind Visibility::getKind(const String& value) { return valueKindMap.find(value)->second; }

} // namespace qat
