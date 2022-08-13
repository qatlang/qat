#include "core_type.hpp"
#include "../../show.hpp"
#include "../qat_module.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"
#include <utility>

namespace qat::IR {

CoreType::CoreType(QatModule *mod, String _name, Vec<Member *> _members,
                   const utils::VisibilityInfo &_visibility,
                   llvm::LLVMContext           &ctx)
    : name(std::move(_name)), parent(mod), members(std::move(_members)),
      visibility(_visibility) {
  SHOW("Generating LLVM Type for core type members")
  Vec<llvm::Type *> subtypes;
  for (auto *mem : members) {
    subtypes.push_back(mem->type->getLLVMType());
  }
  SHOW("All members' LLVM types obtained")
  llvmType = llvm::StructType::create(subtypes, name, false);
  if (mod) {
    mod->coreTypes.push_back(this);
  }
}

String CoreType::getFullName() const {
  return parent->getFullNameWithChild(name);
}

String CoreType::getName() const { return name; }

u64 CoreType::getMemberCount() const { return members.size(); }

bool CoreType::hasMember(const String &member) const {
  for (const auto &mem : members) {
    if (mem->name == member) {
      return true;
    }
  }
  return false;
}

CoreType::Member *CoreType::getMemberAt(u64 index) { return members.at(index); }

Maybe<usize> CoreType::getIndexOf(const String &member) const {
  Maybe<usize> result;
  for (usize i = 0; i < members.size(); i++) {
    if (members.at(i)->name == member) {
      result = i;
      break;
    }
  }
  return result;
}

String CoreType::getMemberNameAt(u64 index) const {
  return (index < members.size()) ? members.at(index)->name : "";
}

QatType *CoreType::getTypeOfMember(const String &member) const {
  Maybe<usize> result;
  for (usize i = 0; i < members.size(); i++) {
    if (members.at(i)->name == member) {
      result = i;
      break;
    }
  }
  if (result) {
    return members.at(result.value())->type;
  } else {
    return nullptr;
  }
}

bool CoreType::hasStatic(const String &_name) const {
  bool result = false;
  for (auto *stm : staticMembers) {
    if (stm->getName() == _name) {
      return true;
    }
  }
  return result;
}

bool CoreType::hasMemberFunction(const String &fnName) const {
  return std::ranges::any_of(
      memberFunctions.begin(), memberFunctions.end(),
      [&](MemberFunction *fun) { return fun->getName() == fnName; });
}

MemberFunction *CoreType::getMemberFunction(const String &fnName) const {
  for (auto *memberFunction : memberFunctions) {
    if (memberFunction->getName() == fnName) {
      return memberFunction;
    }
  }
  return nullptr;
}

bool CoreType::hasStaticFunction(const String &fnName) const {
  return std::ranges::any_of(
      staticFunctions.begin(), staticFunctions.end(),
      [&](MemberFunction *fun) { return fun->getName() == fnName; });
}

const MemberFunction *CoreType::getStaticFunction(const String &fnName) const {
  for (auto *staticFunction : staticFunctions) {
    if (staticFunction->getName() == fnName) {
      return staticFunction;
    }
  }
  return nullptr;
}

void CoreType::addStaticMember(const String &name, QatType *type,
                               bool variability, Value *initial,
                               const utils::VisibilityInfo &visibility,
                               llvm::LLVMContext           &ctx) {
  staticMembers.push_back(
      new StaticMember(this, name, type, variability, initial, visibility));
}

bool CoreType::hasCopyConstructor() const {
  return copyConstructor.has_value();
}

bool CoreType::hasMoveConstructor() const {
  return moveConstructor.has_value();
}

utils::VisibilityInfo CoreType::getVisibility() const { return visibility; }

QatModule *CoreType::getParent() { return parent; }

TypeKind CoreType::typeKind() const { return TypeKind::core; }

String CoreType::toString() const { return getFullName(); }

nuo::Json CoreType::toJson() const {
  return nuo::Json()._("id", getID())._("name", name);
}

} // namespace qat::IR