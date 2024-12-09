#ifndef QAT_IR_ARGUMENT_HPP
#define QAT_IR_ARGUMENT_HPP

#include "../utils/identifier.hpp"
#include "./types/function.hpp"
#include "./types/qat_type.hpp"

namespace qat::ir {

class Argument {
	friend class Function;
	Identifier   name;
	Type*        type;
	bool         variability;
	u64          argIndex;
	ArgumentKind kind;

	Argument(ArgumentKind _kind, Identifier _name, Type* _type, bool _variability, u64 _arg_index)
	    : name(std::move(_name)), type(_type), variability(_variability), argIndex(_arg_index), kind(_kind) {}

  public:
	useit static Argument Create(const Identifier& name, Type* type, u64 arg_index) {
		return {ArgumentKind::NORMAL, name, type, false, arg_index};
	}

	useit static Argument CreateMember(const Identifier& name, Type* type, u64 argIndex) {
		return {ArgumentKind::MEMBER, name, type, true, argIndex};
	}

	useit static Argument CreateVariable(const Identifier& name, Type* type, u64 arg_index) {
		return {ArgumentKind::NORMAL, name, type, true, arg_index};
	}

	useit static Argument CreateVariadic(String name, FileRange range, u64 argIndex) {
		return {ArgumentKind::VARIADIC, {name, range}, nullptr, false, argIndex};
	}

	useit Identifier get_name() const { return name; }

	useit ArgumentType* to_arg_type() const {
		switch (kind) {
			case ArgumentKind::NORMAL:
				return ArgumentType::create_normal(type, name.value, variability);
			case ArgumentKind::MEMBER:
				return ArgumentType::create_member(name.value, type);
			case ArgumentKind::VARIADIC:
				return ArgumentType::create_variadic(name.value.empty() ? None : Maybe<String>(name.value));
		}
	}

	useit Type* get_type() const { return type; }
	useit bool  get_variability() const { return variability; }
	useit u64   get_arg_index() const { return argIndex; }
	useit Json  to_json() const {
        return Json()
            ._("name", name)
            ._("index", argIndex)
            ._("hasType", type != nullptr)
            ._("type", type ? type->get_id() : JsonValue())
		     ._("isVar", variability)
            ._("kind",
               kind == ArgumentKind::MEMBER ? "member" : (kind == ArgumentKind::NORMAL ? "normal" : "variadic"));
	}
};

} // namespace qat::ir

#endif
