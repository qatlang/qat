#include "./tuple.hpp"

namespace qat::IR {

TupleType::TupleType(const std::vector<QatType *> _types, const bool _isPacked)
    : types(_types), isPacked(_isPacked) {}

std::vector<QatType *> TupleType::getSubTypes() const { return types; }

QatType *TupleType::getSubtypeAt(unsigned index) { return types.at(index); }

unsigned TupleType::getSubTypeCount() const { return types.size(); }

bool TupleType::isPackedTuple() const { return isPacked; }

TypeKind TupleType::typeKind() const { return TypeKind::tuple; }

std::string TupleType::toString() const {
  std::string result = "(";
  for (std::size_t i = 0; i < types.size(); i++) {
    result += types.at(i)->toString();
    if (i != (types.size() - 1)) {
      result += ", ";
    }
  }
  result += ")";
}

} // namespace qat::IR