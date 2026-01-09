#ifndef QAT_AST_ARGUMENT_HPP
#define QAT_AST_ARGUMENT_HPP

#include "../utils/macros.hpp"
#include "./types/qat_type.hpp"

namespace qat::ast {

enum class ArgKind {
	NORMAL,
	MEMBER,
};

inline String arg_kind_to_string(ArgKind kind) {
	switch (kind) {
		case ArgKind::NORMAL:
			return "normal";
		case ArgKind::MEMBER:
			return "member";
	}
}

class Argument {
  private:
	bool       isVar;
	Identifier name;
	Type*      type;
	ArgKind    kind;

  public:
	Argument(ArgKind _kind, Identifier _name, bool _isVar, Type* _type)
	    : isVar(_isVar), name(_name), type(_type), kind(_kind) {}

	static Argument* create_normal(Identifier name, bool isVar, Type* type) {
		return std::construct_at(OwnNormal(Argument), ArgKind::NORMAL, name, isVar, type);
	}

	static Argument* create_member(Identifier name, bool isVar, Type* type) {
		return std::construct_at(OwnNormal(Argument), ArgKind::MEMBER, name, isVar, type);
	}

	Identifier get_name() const { return name; }

	bool is_variable() const { return isVar; }

	Type* get_type() { return type; }

	bool is_member_arg() const { return kind == ArgKind::MEMBER; }
};

} // namespace qat::ast

#endif
