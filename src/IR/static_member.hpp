#ifndef QAT_IR_STATIC_MEMBER_HPP
#define QAT_IR_STATIC_MEMBER_HPP

#include "../utils/identifier.hpp"
#include "../utils/visibility.hpp"
#include "./entity_overview.hpp"
#include "./value.hpp"

namespace qat::ir {

class StructType;
class Mod;

class StaticMember final : public Value, public EntityOverview {
	Identifier     name;
	StructType*    parent;
	Value*         initial;
	VisibilityInfo visibility;

  public:
	StaticMember(StructType* _parent, Identifier name, Type* _type, bool _is_variable, Value* _initial,
	             const VisibilityInfo& _visibility);

	useit static StaticMember* get(StructType* _parent, Identifier name, Type* _type, bool _is_variable,
	                               Value* _initial, const VisibilityInfo& _visibility) {
		return std::construct_at(OwnNormal(StaticMember), _parent, name, _type, _is_variable, _initial, _visibility);
	}

	useit StructType* get_parent_type();

	useit Identifier get_name() const;

	useit String get_full_name() const;

	useit const VisibilityInfo& get_visibility() const;

	useit bool has_initial() const;

	useit Value* get_initial() const { return initial; }

	useit Json to_json() const;

	void update_overview() final;

	~StaticMember() final = default;
};

} // namespace qat::ir

#endif
