#include "./argument.hpp"

namespace qat::ast {

Argument::Argument(std::string _name, utils::FileRange _fileRange,
                   QatType *_type, bool _isMember)
    : name(_name), fileRange(_fileRange), type(_type), isMember(_isMember) {}

Argument *Argument::Normal(std::string name, utils::FileRange fileRange,
                           QatType *type) {
  return new Argument(name, fileRange, type, false);
}

Argument *Argument::ForConstructor(std::string name, utils::FileRange fileRange,
                                   QatType *type, bool isMember) {
  return new Argument(name, fileRange, type, isMember);
}

std::string Argument::getName() const { return name; }

utils::FileRange Argument::getFileRange() const { return fileRange; }

QatType *Argument::getType() { return type; }

bool Argument::isTypeMember() const { return isMember; }

} // namespace qat::ast