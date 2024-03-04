#include "./global_entity.hpp"
#include "./qat_module.hpp"
#include "entity_overview.hpp"

namespace qat::ir {

PrerunGlobal::PrerunGlobal(Mod* _parent, Identifier _name, Type* _type, llvm::Constant* _constant,
                           VisibilityInfo _visibility, FileRange _fileRange)
    : PrerunValue(_constant, _type), EntityOverview("prerunGlobal", Json(), _fileRange), name(_name),
      visibility(_visibility), parent(_parent) {
  parent->prerunGlobals.push_back(this);
}

String PrerunGlobal::get_full_name() const { return parent->get_fullname_with_child(name.value); }

GlobalEntity::GlobalEntity(Mod* _parent, Identifier _name, Type* _type, bool _is_variable,
                           Maybe<llvm::Constant*> _initialValue, llvm::Value* _value, const VisibilityInfo& _visibility)
    : Value(_value, _type, _is_variable), EntityOverview("global", Json(), _name.range), name(std::move(_name)),
      visibility(_visibility), parent(_parent), initialValue(_initialValue) {
  parent->globalEntities.push_back(this);
}

ir::Mod* GlobalEntity::get_module() const { return parent; }

Identifier GlobalEntity::get_name() const { return name; }

String GlobalEntity::get_full_name() const { return parent->get_fullname_with_child(name.value); }

bool GlobalEntity::hasInitialValue() const { return initialValue.has_value(); }

llvm::Constant* GlobalEntity::getInitialValue() const { return initialValue.value(); }

const VisibilityInfo& GlobalEntity::get_visibility() const { return visibility; }

} // namespace qat::ir