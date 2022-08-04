#include "./static_member.hpp"
#include "./types/core_type.hpp"

namespace qat::IR {

StaticMember::StaticMember(CoreType *_parent, String _name, QatType *_type,
                           bool _isVariable, Value *_initial,
                           const utils::VisibilityInfo &_visibility)
    : parent(_parent), name(std::move(_name)), initial(_initial),
      visibility(_visibility), Value(_type, _isVariable, Nature::assignable),
      loads(0), stores(0), refers(0) {}

CoreType *StaticMember::getParentType() { return parent; }

String StaticMember::getName() const { return name; }

String StaticMember::getFullName() const {
  return parent->getFullName() + ":" + name;
}

const utils::VisibilityInfo &StaticMember::getVisibility() const {
  return visibility;
}

bool StaticMember::hasInitial() const { return (initial != nullptr); }

u64 StaticMember::getLoadCount() const { return loads; }

u64 StaticMember::getStoreCount() const { return stores; }

u64 StaticMember::getReferCount() const { return refers; }

llvm::Value *StaticMember::emitLLVM(llvmHelper &helper) const {
  // TODO - Implement
}

void StaticMember::emitCPP(cpp::File &file) const {
  // TODO - Implement
}
nuo::Json StaticMember::toJson() const {
  // TODO - Implement
}

} // namespace qat::IR