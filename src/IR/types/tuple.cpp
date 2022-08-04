#include "./tuple.hpp"

namespace qat::IR {

TupleType::TupleType(const Vec<QatType *> _types, const bool _isPacked)
    : types(_types), isPacked(_isPacked) {}

Vec<QatType *> TupleType::getSubTypes() const { return types; }

QatType *TupleType::getSubtypeAt(u64 index) { return types.at(index); }

u64 TupleType::getSubTypeCount() const { return types.size(); }

bool TupleType::isPackedTuple() const { return isPacked; }

TypeKind TupleType::typeKind() const { return TypeKind::tuple; }

String TupleType::toString() const {
  String result = "(";
  for (usize i = 0; i < types.size(); i++) {
    result += types.at(i)->toString();
    if (i != (types.size() - 1)) {
      result += ", ";
    }
  }
  result += ")";
}

} // namespace qat::IR