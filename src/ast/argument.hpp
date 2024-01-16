#ifndef QAT_AST_ARGUMENT_HPP
#define QAT_AST_ARGUMENT_HPP

#include "../utils/file_range.hpp"
#include "../utils/macros.hpp"
#include "./types/qat_type.hpp"
#include <optional>
#include <string>

namespace qat::ast {

class Argument {
private:
  bool       isVar;
  Identifier name;
  QatType*   type;
  bool       isMember;

public:
  Argument(Identifier _name, bool _isVar, QatType* _type, bool _isMember)
      : isVar(_isVar), name(std::move(_name)), type(_type), isMember(_isMember) {}

  static Argument* Normal(Identifier name, bool isVar, QatType* type) {
    return std::construct_at(OwnNormal(Argument), name, isVar, type, false);
  }

  static Argument* ForConstructor(Identifier name, bool isVar, QatType* type, bool isMember) {
    return std::construct_at(OwnNormal(Argument), name, isVar, type, isMember);
  }

  useit inline Identifier getName() const { return name; }
  useit inline bool       isVariable() const { return isVar; }
  useit inline QatType*   getType() { return type; }
  useit inline bool       isTypeMember() const { return isMember; }
};

} // namespace qat::ast

#endif