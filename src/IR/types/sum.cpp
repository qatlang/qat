#include "./sum.hpp"
#include "../qat_module.hpp"
#include "qat_type.hpp"
#include "type_kind.hpp"

namespace qat::IR {

SumType::SumType(String _name, Vec<QatType *> _subTypes)
    : name(std::move(_name)), subTypes(std::move(_subTypes)) {}

String SumType::getName() const { return name; }

String SumType::getFullName() const {
  return parent->getFullName() + ":" + name;
}

u32 SumType::getSubtypeCount() const { return subTypes.size(); }

QatType *SumType::getSubtypeAt(usize index) { return subTypes.at(index); }

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

String SumType::toString() const { return name; }

} // namespace qat::IR