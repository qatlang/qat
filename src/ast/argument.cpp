#include "./argument.hpp"

namespace qat::ast {

Argument::Argument(String _name, utils::FileRange _fileRange, QatType *_type,
                   bool _isMember)
    : name(std::move(_name)), fileRange(std::move(_fileRange)), type(_type),
      isMember(_isMember) {}

Argument *Argument::Normal(String name, utils::FileRange fileRange,
                           QatType *type) {
  return new Argument(std::move(name), std::move(fileRange), type, false);
}

Argument *Argument::ForConstructor(String name, utils::FileRange fileRange,
                                   QatType *type, bool isMember) {
  return new Argument(std::move(name), std::move(fileRange), type, isMember);
}

auto Argument::getName() -> String const { return name; }

utils::FileRange Argument::getFileRange() const { return fileRange; }

auto Argument::getType() -> QatType * { return type; }

bool Argument::isTypeMember() const { return isMember; }

} // namespace qat::ast