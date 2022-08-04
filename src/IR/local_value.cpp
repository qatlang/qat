#include "./local_value.hpp"
#include "../ast/types/qat_type.hpp"

namespace qat::IR {

LocalValue::LocalValue(String _name, QatType *_type, bool is_variable,
                       Value *_initial, Block *block)
    : name(_name), initial(_initial), parent(block), loads(0), stores(0),
      refers(0), Value(_type, is_variable, Nature::assignable) {}

bool LocalValue::isRemovable() const {
  if (getType()->typeKind() != IR::TypeKind::reference) {
    return (getType()->typeKind() != IR::TypeKind::pointer && (loads == 0) &&
                (refers == 0) ||
            ((loads == 0) && (stores == 0) && (refers == 0)));
  } else {
    return (loads == 0) && (stores == 0) && (refers == 0);
  }
}

String LocalValue::getName() const { return name; }

u64 LocalValue::getLoads() const { return loads; }

u64 LocalValue::getStores() const { return stores; }

u64 LocalValue::getRefers() const { return refers; }

Block *LocalValue::getParent() { return parent; }

} // namespace qat::IR