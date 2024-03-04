#ifndef QAT_AST_ARGUMENT_HPP
#define QAT_AST_ARGUMENT_HPP

#include "../utils/macros.hpp"
#include "./types/qat_type.hpp"

namespace qat::ast {

class Argument {
private:
  bool       isVar;
  Identifier name;
  Type*      type;
  bool       isMember;

public:
  Argument(Identifier _name, bool _isVar, Type* _type, bool _isMember)
      : isVar(_isVar), name(_name), type(_type), isMember(_isMember) {}

  static Argument* create_normal(Identifier name, bool isVar, Type* type) {
    return std::construct_at(OwnNormal(Argument), name, isVar, type, false);
  }

  static Argument* create_for_constructor(Identifier name, bool isVar, Type* type, bool isMember) {
    return std::construct_at(OwnNormal(Argument), name, isVar, type, isMember);
  }

  useit inline Identifier get_name() const { return name; }
  useit inline bool       is_variable() const { return isVar; }
  useit inline Type*      get_type() { return type; }
  useit inline bool       is_type_member() const { return isMember; }
};

} // namespace qat::ast

#endif