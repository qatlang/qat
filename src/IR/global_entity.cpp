#include "./global_entity.hpp"
#include "./qat_module.hpp"

namespace qat::IR {

GlobalEntity::GlobalEntity(QatModule *_parent, String _name, QatType *_type,
                           bool _is_variable, llvm::Value *_value,
                           const utils::VisibilityInfo &_visibility)
    : Value(_value, _type, _is_variable, Nature::assignable),
      name(std::move(_name)), visibility(_visibility), parent(_parent) {
  parent->globalEntities.push_back(this);
}

String GlobalEntity::getName() const { return name; }

String GlobalEntity::getFullName() const {
  return parent->getFullNameWithChild(name);
}

const utils::VisibilityInfo &GlobalEntity::getVisibility() const {
  return visibility;
}

u64 GlobalEntity::getLoadCount() const { return loads; }

u64 GlobalEntity::getStoreCount() const { return stores; }

u64 GlobalEntity::getReferCount() const { return refers; }

Json GlobalEntity::toJson() const {
  return Json()
      ._("name", name)
      ._("type", type->toJson())
      ._("isVariable", variable)
      ._("visibility", visibility);
}

} // namespace qat::IR