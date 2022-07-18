#include "./argument.hpp"

namespace qat::AST {

Argument::Argument(std::string _name, utils::FileRange _placement,
                   QatType *_type, bool _isMember)
    : name(_name), placement(_placement), type(_type), isMember(_isMember) {}

Argument *Argument::Normal(std::string name, utils::FileRange placement,
                           QatType *type) {
  return new Argument(name, placement, type, false);
}

Argument *Argument::ForConstructor(std::string name, utils::FileRange placement,
                                   QatType *type, bool isMember) {
  return new Argument(name, placement, type, isMember);
}

std::string Argument::getName() const { return name; }

utils::FileRange Argument::getFileRange() const { return placement; }

QatType *Argument::getType() { return type; }

bool Argument::isTypeMember() const { return isMember; }

} // namespace qat::AST