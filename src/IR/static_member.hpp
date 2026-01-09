#ifndef QAT_IR_STATIC_MEMBER_HPP
#define QAT_IR_STATIC_MEMBER_HPP

#include "../utils/identifier.hpp"
#include "../utils/mentionable.hpp"
#include "../utils/visibility.hpp"
#include "./value.hpp"

namespace qat::ir {

class StructType;
class Mod;

class StaticMember final : public Value, public Mentionable {
	Identifier     name;
	StructType*    parent;
	Value*         initial;
	VisibilityInfo visibility;

  public:
	StaticMember(StructType* _parent, Identifier name, Type* _type, bool _is_variable, Value* _initial,
	             const VisibilityInfo& _visibility);

	static StaticMember* get(StructType* _parent, Identifier name, Type* _type, bool _is_variable, Value* _initial,
	                         const VisibilityInfo& _visibility) {
		return std::construct_at(OwnNormal(StaticMember), _parent, name, _type, _is_variable, _initial, _visibility);
	}

	StructType* get_parent_type();

	Identifier get_name() const;

	String get_full_name() const;

	const VisibilityInfo& get_visibility() const;

	bool has_initial() const;

	Value* get_initial() const { return initial; }

	~StaticMember() final = default;
};

} // namespace qat::ir

#endif
