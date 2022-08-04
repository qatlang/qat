#include <utility>

#include "../qat_module.hpp"
#include "llvm/IR/DerivedTypes.h"

namespace qat::IR {

CoreType::CoreType(QatModule *mod, String _name, Vec<Member *> _members,
                   Vec<MemberFunction *>        _memberFunctions,
                   const utils::VisibilityInfo &_visibility)
    : name(std::move(_name)), members(std::move(_members)), staticMembers(),
      memberFunctions(std::move(_memberFunctions)),
      destructor(MemberFunction::CreateDestructor(
          this, utils::FileRange("", {0u, 0u}, {0u, 0u}))),
      visibility(_visibility), parent(mod) {}

String CoreType::getFullName() const {
  return parent->getFullNameWithChild(name);
}

String CoreType::getName() const {
  auto full_name = getFullName();
  if (full_name.find(':') != String::npos) {
    auto result = full_name.substr(full_name.find_last_of(':') + 1);
    return result;
  } else {
    return getFullName();
  }
}

u64 CoreType::getMemberCount() const { return members.size(); }

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
  for (auto st : staticMembers) {
    if (st->getName() == _name) {
      return true;
    }
  }
  return result;
}

bool CoreType::hasMemberFunction(const String &fnName) const {
  return std::ranges::any_of(
      memberFunctions.begin(), memberFunctions.end(),
      [&](MemberFunction *fn) { return fn->getName() == fnName; });
}

const MemberFunction *CoreType::getMemberFunction(const String &fnName) const {
  for (auto memberFunction : memberFunctions) {
    if (memberFunction->getName() == fnName) {
      return memberFunction;
    }
  }
  return nullptr;
}

bool CoreType::hasStaticFunction(const String &fnName) const {
  return std::ranges::any_of(
      staticFunctions.begin(), staticFunctions.end(),
      [&](MemberFunction *fn) { return fn->getName() == fnName; });
}

const MemberFunction *CoreType::getStaticFunction(const String &fnName) const {
  for (auto staticFunction : staticFunctions) {
    if (staticFunction->getName() == fnName) {
      return staticFunction;
    }
  }
  return nullptr;
}

void CoreType::addMemberFunction(const String &name, const bool is_variation,
                                 QatType *  return_type,
                                 const bool is_return_type_variable,
                                 const bool is_async, Vec<Argument> args,
                                 const bool                   has_variadic_args,
                                 const utils::FileRange &     fileRange,
                                 const utils::VisibilityInfo &visibilityInfo) {
  memberFunctions.push_back(MemberFunction::Create(
      this, is_variation, name, return_type, is_return_type_variable, is_async,
      std::move(args), has_variadic_args, fileRange, visibilityInfo));
}

void CoreType::addStaticFunction(const String &name, QatType *return_type,
                                 const bool is_return_type_variable,
                                 const bool is_async, Vec<Argument> args,
                                 const bool                   has_variadic_args,
                                 const utils::FileRange &     fileRange,
                                 const utils::VisibilityInfo &visibilityInfo) {
  staticFunctions.push_back(MemberFunction::CreateStatic(
      this, name, return_type, is_return_type_variable, is_async,
      std::move(args), has_variadic_args, fileRange, visibilityInfo));
}

void CoreType::addStaticMember(const String &name, QatType *type,
                               bool variability, Value *initial,
                               const utils::VisibilityInfo &visibility) {
  staticMembers.push_back(
      new StaticMember(this, name, type, variability, initial, visibility));
}

utils::VisibilityInfo CoreType::getVisibility() const { return visibility; }

QatModule *CoreType::getParent() { return parent; }

TypeKind CoreType::typeKind() const { return TypeKind::core; }

String CoreType::toString() const { return getFullName(); }

void CoreType::defineLLVM(llvmHelper &helper) const {
  Vec<llvm::Type *> genMemTypes;
  for (auto mem : members) {
    genMemTypes.push_back(mem->type->emitLLVM(helper));
  }
  llvm::StructType::create(helper.llctx, genMemTypes, getFullName(), false);
}

void CoreType::defineCPP(cpp::File &file) const {
  file << "class " << getName() << " {\n";
  // FIXME - Implement this
  file << "\n}";
}

llvm::Type *CoreType::emitLLVM(llvmHelper &help) const {
  return llvm::StructType::getTypeByName(help.llctx, getFullName());
}

void CoreType::emitCPP(cpp::File &file) const { file << getFullName() << " "; }

nuo::Json CoreType::toJson() const {
  return nuo::Json()._("id", getID())._("name", name);
}

} // namespace qat::IR