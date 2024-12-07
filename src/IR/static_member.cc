#include "./static_member.hpp"
#include "./qat_module.hpp"
#include "./types/struct_type.hpp"
#include "entity_overview.hpp"

namespace qat::ir {

StaticMember::StaticMember(StructType* _parent, Identifier _name, Type* _type, bool _isVariable, Value* _initial,
						   const VisibilityInfo& _visibility)
	: Value(nullptr, _type, _isVariable), EntityOverview("staticMember", Json(), _name.range), name(std::move(_name)),
	  parent(_parent), initial(_initial), visibility(_visibility) {
	// TODO
}

StructType* StaticMember::get_parent_type() { return parent; }

Identifier StaticMember::get_name() const { return name; }

String StaticMember::get_full_name() const { return parent->get_full_name() + ":" + name.value; }

const VisibilityInfo& StaticMember::get_visibility() const { return visibility; }

bool StaticMember::has_initial() const { return (initial != nullptr); }

Json StaticMember::to_json() const {
	// TODO - Implement
	return Json();
}

void StaticMember::update_overview() {
	ovInfo._("parentID", parent->get_id())
		._("typeID", get_ir_type()->get_id())
		._("type", type->to_string())
		._("name", name)
		._("fullName", get_full_name())
		._("visibility", visibility)
		._("isVariable", is_variable())
		._("moduleID", parent->get_module()->get_id());
}

} // namespace qat::ir
