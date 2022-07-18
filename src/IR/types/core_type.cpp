#include "./core_type.hpp"
#include "../qat_module.hpp"

namespace qat::IR {

CoreType::CoreType(QatModule *mod, const std::string _name,
                   const std::vector<Member *> _members,
                   const std::vector<MemberFunction *> _memberFunctions,
                   const utils::VisibilityInfo _visibility)
    : name(_name), members(_members), static_members(),
      memberFunctions(_memberFunctions),
      destructor(MemberFunction::CreateDestructor(
          this, utils::FileRange("", {0u, 0u}, {0u, 0u}))),
      visibility(_visibility), parent(mod) {}

std::string CoreType::getFullName() const {
  return parent->getFullNameWithChild(name);
}

std::string CoreType::getName() const {
  auto full_name = getFullName();
  if (full_name.find(':') != std::string::npos) {
    auto result = full_name.substr(full_name.find_last_of(':') + 1);
    return result;
  } else {
    return getFullName();
  }
}

unsigned CoreType::getMemberCount() const { return members.size(); }

CoreType::Member *CoreType::getMemberAt(unsigned index) {
  return members.at(index);
}

int CoreType::getIndexOf(const std::string member) const {
  auto result = -1;
  for (std::size_t i = 0; i < members.size(); i++) {
    if (members.at(i)->name == member) {
      result = i;
      break;
    }
  }
  return result;
}

std::string CoreType::getMemberNameAt(const unsigned int index) const {
  return (index < members.size()) ? members.at(index)->name : "";
}

QatType *CoreType::getTypeOfMember(const std::string member) const {
  auto result = -1;
  for (std::size_t i = 0; i < members.size(); i++) {
    if (members.at(i)->name == member) {
      result = i;
      break;
    }
  }
  if (result != -1) {
    return members.at(result)->type;
  } else {
    return nullptr;
  }
}

bool CoreType::hasStatic(const std::string _name) const {
  bool result = false;
  for (auto st : static_members) {
    if (st->getName() == _name) {
      return true;
    }
  }
  return result;
}

bool CoreType::hasMemberFunction(const std::string fnName) const {
  for (std::size_t i = 0; i < memberFunctions.size(); i++) {
    if (memberFunctions.at(i)->getName() == fnName) {
      return true;
    }
  }
  return false;
}

const MemberFunction *
CoreType::getMemberFunction(const std::string fnName) const {
  for (std::size_t i = 0; i < memberFunctions.size(); i++) {
    if (memberFunctions.at(i)->getName() == fnName) {
      return memberFunctions.at(i);
    }
  }
}

bool CoreType::hasStaticFunction(const std::string fnName) const {
  for (std::size_t i = 0; i < staticFunctions.size(); i++) {
    if (staticFunctions.at(i)->getName() == fnName) {
      return true;
    }
  }
  return false;
}

const MemberFunction *
CoreType::getStaticFunction(const std::string fnName) const {
  for (std::size_t i = 0; i < staticFunctions.size(); i++) {
    if (staticFunctions.at(i)->getName() == fnName) {
      return staticFunctions.at(i);
    }
  }
}

void CoreType::addMemberFunction(
    const std::string name, const bool is_variation, QatType *return_type,
    const bool is_return_type_variable, const bool is_async,
    const std::vector<Argument> args, const bool has_variadic_args,
    const utils::FileRange placement, const utils::VisibilityInfo visib_info) {
  memberFunctions.push_back(MemberFunction::Create(
      this, is_variation, name, return_type, is_return_type_variable, is_async,
      args, has_variadic_args, placement, visib_info));
}

void CoreType::addStaticFunction(const std::string name, QatType *return_type,
                                 const bool is_return_type_variable,
                                 const bool is_async,
                                 const std::vector<Argument> args,
                                 const bool has_variadic_args,
                                 const utils::FileRange placement,
                                 const utils::VisibilityInfo visib_info) {
  staticFunctions.push_back(MemberFunction::CreateStatic(
      this, name, return_type, is_return_type_variable, is_async, args,
      has_variadic_args, placement, visib_info));
}

void CoreType::addStaticMember(std::string name, QatType *type,
                               bool variability, Value *initial,
                               utils::VisibilityInfo visibility) {
  static_members.push_back(
      new StaticMember(this, name, type, variability, initial, visibility));
}

utils::VisibilityInfo CoreType::getVisibility() const { return visibility; }

QatModule *CoreType::getParent() { return parent; }

TypeKind CoreType::typeKind() const { return TypeKind::core; }

std::string CoreType::toString() const { return getFullName(); }

void CoreType::defineLLVM(llvmHelper helper) const {
  // TODO - Implement this
}

void CoreType::defineCPP(backend::cpp::File &file) const {
  // TODO - Implement this
}

nuo::Json CoreType::toJson() const {
  // TODO - Implement this
  return nuo::Json();
}

} // namespace qat::IR