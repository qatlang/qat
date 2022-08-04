#include "./global_entity.hpp"
#include "./qat_module.hpp"

namespace qat::IR {

GlobalEntity::GlobalEntity(QatModule *_parent, String _name, QatType *_type,
                           bool _is_variable, Value *_value,
                           utils::VisibilityInfo _visibility)
    : parent(_parent), name(_name), initial(_value), visibility(_visibility),
      Value(_type, _is_variable, Nature::assignable) {}

String GlobalEntity::getName() const { return name; }

String GlobalEntity::getFullName() const {
  return parent->getFullNameWithChild(name);
}

const utils::VisibilityInfo &GlobalEntity::getVisibility() const {
  return visibility;
}

bool GlobalEntity::hasInitial() const { return (initial != nullptr); }

u64 GlobalEntity::getLoadCount() const { return loads; }

u64 GlobalEntity::getStoreCount() const { return stores; }

u64 GlobalEntity::getReferCount() const { return refers; }

void GlobalEntity::defineLLVM(llvmHelper &help) const {}

void GlobalEntity::defineCPP(cpp::File &file) const {
  if (file.isHeaderFile()) {
    if (!isVariable()) {
      file << "const ";
    }
    type->emitCPP(file);
    file << (" " + name + ";\n");
  } else {
    if (initial) {
      if (!isVariable()) {
        file << "const ";
      }
      type->emitCPP(file);
      file << (" " + name + " = ");
      initial->emitCPP(file);
    }
  }
}

nuo::Json GlobalEntity::toJson() const {}

} // namespace qat::IR