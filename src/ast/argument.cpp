#include "./argument.hpp"

namespace qat::ast {

Argument::Argument(Identifier _name, QatType* _type, bool _isMember)
    : name(std::move(_name)), type(_type), isMember(_isMember) {}

Argument* Argument::Normal(Identifier name, QatType* type) { return new Argument(std::move(name), type, false); }

Argument* Argument::ForConstructor(Identifier name, QatType* type, bool isMember) {
  return new Argument(std::move(name), type, isMember);
}

Identifier Argument::getName() const { return name; }

auto Argument::getType() -> QatType* { return type; }

bool Argument::isTypeMember() const { return isMember; }

} // namespace qat::ast