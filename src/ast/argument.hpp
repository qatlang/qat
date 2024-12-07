#ifndef QAT_AST_ARGUMENT_HPP
#define QAT_AST_ARGUMENT_HPP

#include "../utils/macros.hpp"
#include "./types/qat_type.hpp"

namespace qat::ast {

enum class ArgKind {
	NORMAL,
	MEMBER,
	VARIADIC,
};

useit inline String arg_kind_to_string(ArgKind kind) {
	switch (kind) {
		case ArgKind::NORMAL:
			return "normal";
		case ArgKind::MEMBER:
			return "member";
		case ArgKind::VARIADIC:
			return "variadic";
	}
}

class Argument {
  private:
	bool	   isVar;
	Identifier name;
	Type*	   type;
	ArgKind	   kind;

  public:
	Argument(ArgKind _kind, Identifier _name, bool _isVar, Type* _type)
		: isVar(_isVar), name(_name), type(_type), kind(_kind) {}

	useit static Argument* create_normal(Identifier name, bool isVar, Type* type) {
		return std::construct_at(OwnNormal(Argument), ArgKind::NORMAL, name, isVar, type);
	}

	useit static Argument* create_member(Identifier name, bool isVar, Type* type) {
		return std::construct_at(OwnNormal(Argument), ArgKind::MEMBER, name, isVar, type);
	}

	useit static Argument* create_variadic(FileRange range) {
		return std::construct_at(OwnNormal(Argument), ArgKind::VARIADIC, Identifier{"", range}, false, nullptr);
	}

	useit Identifier get_name() const { return name; }
	useit bool		 is_variable() const { return isVar; }
	useit Type*		 get_type() { return type; }

	useit bool is_member_arg() const { return kind == ArgKind::MEMBER; }
	useit bool is_variadic_arg() const { return kind == ArgKind::VARIADIC; }

	useit Json to_json() const {
		return Json()
			._("name", name)
			._("isVar", isVar)
			._("hasType", type != nullptr)
			._("type", (type != nullptr) ? type->to_json() : Json())
			._("kind", arg_kind_to_string(kind));
	}
};

} // namespace qat::ast

#endif
