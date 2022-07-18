#include "./local_value.hpp"
#include "../AST/types/qat_type.hpp"

namespace qat::IR {

LocalValue::LocalValue(std::string _name, QatType *_type, bool is_variable,
                       Value *_initial, Block *block)
    : name(_name), initial(_initial), parent(block), loads(0), stores(0),
      refers(0), Value(_type, is_variable, Kind::assignable) {}

bool LocalValue::isRemovable() const {
  if (getType()->typeKind() != IR::TypeKind::reference) {
    return (getType()->typeKind() != IR::TypeKind::pointer && (loads == 0) &&
                (refers == 0) ||
            ((loads == 0) && (stores == 0) && (refers == 0)));
  } else {
    return (loads == 0) && (stores == 0) && (refers == 0);
  }
}

Block *LocalValue::getParent() { return parent; }

} // namespace qat::IR