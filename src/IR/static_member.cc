#include "./static_member.hpp"
#include "./qat_module.hpp"
#include "./types/struct_type.hpp"

namespace qat::ir {

StaticMember::StaticMember(StructType* _parent, Identifier _name, Type* _type, bool _isVariable, Value* _initial,
                           const VisibilityInfo& _visibility)
    : Value(nullptr, _type, _isVariable), name(std::move(_name)), parent(_parent), initial(_initial),
      visibility(_visibility) {
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
} // namespace qat::ir
