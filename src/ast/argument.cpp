#include "./argument.hpp"

namespace qat::ast {

Argument::Argument(Identifier _name, bool _isVar, QatType* _type, bool _isMember)
    : isVar(_isVar), name(std::move(_name)), type(_type), isMember(_isMember) {}

Argument* Argument::Normal(Identifier name, bool isVar, QatType* type) {
  return new Argument(std::move(name), isVar, type, false);
}

Argument* Argument::ForConstructor(Identifier name, bool isVar, QatType* type, bool isMember) {
  return new Argument(std::move(name), isVar, type, isMember);
}

Identifier Argument::getName() const { return name; }

bool Argument::isVariable() const { return isVar; }

auto Argument::getType() -> QatType* { return type; }

bool Argument::isTypeMember() const { return isMember; }

} // namespace qat::ast