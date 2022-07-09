#include "./sum.hpp"
#include "../qat_module.hpp"
#include "qat_type.hpp"
#include "type_kind.hpp"

namespace qat {
namespace IR {

SumType::SumType(std::string _name, std::vector<QatType *> _subTypes)
    : name(_name), subTypes(_subTypes) {}

std::string SumType::getName() const { return name; }

std::string SumType::getFullName() const {
  return parent->getFullName() + ":" + name;
}

unsigned SumType::getSubtypeCount() const { return subTypes.size(); }

QatType *SumType::getSubtypeAt(unsigned int index) {
  return subTypes.at(index);
}

bool SumType::isCompatible(QatType *other) const {
  if (isSame(other)) {
    return true;
  }
  for (auto typ : subTypes) {
    if (typ->isSame(other)) {
      return true;
    } else if (typ->typeKind() == TypeKind::sumType) {
      if (((SumType *)typ)->isCompatible(other)) {
        return true;
      }
    }
  }
  return false;
}

} // namespace IR
} // namespace qat