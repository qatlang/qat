#include "core_type.hpp"
#include "../../ast/define_core_type.hpp"
#include "../../ast/types/templated.hpp"
#include "../../show.hpp"
#include "../../utils/split_string.hpp"
#include "../qat_module.hpp"
#include "definition.hpp"
#include "function.hpp"
#include "reference.hpp"
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
  llvmType = llvm::StructType::create(subtypes, mod->getFullNameWithChild(name),
                                      false);
  if (mod) {
    mod->coreTypes.push_back(this);
  }
}

String CoreType::getFullName() const {
  return parent->getFullNameWithChild(name);
}

String CoreType::getName() const { return name; }

u64 CoreType::getMemberCount() const { return members.size(); }

Vec<CoreType::Member *> &CoreType::getMembers() { return members; }

bool CoreType::hasMember(const String &member) const {
  for (auto *mem : members) {
    if (mem->name == member) {
      return true;
    }
  }
  return false;
}

CoreType::Member *CoreType::getMember(const String &member) const {
  for (auto *mem : members) {
    if (mem->name == member) {
      return mem;
    }
  }
  return nullptr;
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
  for (auto *memberFunction : memberFunctions) {
    SHOW("Found member function: " << memberFunction->getName())
    if (memberFunction->getName() == fnName) {
      return true;
    }
  }
  return false;
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

MemberFunction *CoreType::getStaticFunction(const String &fnName) const {
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

bool CoreType::hasBinaryOperator(String opr, IR::QatType *type) const {
  for (auto *bin : binaryOperators) {
    if (utils::splitString(bin->getName(), "'")[1] == opr) {
      auto *binArgTy =
          bin->getType()->asFunction()->getArgumentTypeAt(1)->getType();
      if (binArgTy->isSame(type) ||
          (binArgTy->isReference() &&
           binArgTy->asReference()->getSubType()->isSame(type))) {
        return true;
      }
    }
  }
  return false;
}

MemberFunction *CoreType::getBinaryOperator(String       opr,
                                            IR::QatType *type) const {
  for (auto *bin : binaryOperators) {
    if (utils::splitString(bin->getName(), "'")[1] == opr) {
      auto *binArgTy =
          bin->getType()->asFunction()->getArgumentTypeAt(1)->getType();
      if (binArgTy->isSame(type) ||
          (binArgTy->isReference() &&
           binArgTy->asReference()->getSubType()->isSame(type))) {
        return bin;
      }
    }
  }
  return nullptr;
}

bool CoreType::hasUnaryOperator(String opr) const {
  for (auto *unr : unaryOperators) {
    if (utils::splitString(unr->getName(), "'")[1] == opr) {
      return true;
    }
  }
  return false;
}

MemberFunction *CoreType::getUnaryOperator(String opr) const {
  for (auto *unr : unaryOperators) {
    if (utils::splitString(unr->getName(), "'")[1] == opr) {
      return unr;
    }
  }
  return nullptr;
}

u64 CoreType::getOperatorVariantIndex(String opr) const {
  u64 index = 0;
  for (auto *bin : binaryOperators) {
    if (utils::splitString(bin->getName(), "'")[1] == opr) {
      index++;
    }
  }
  return index;
}

bool CoreType::hasAnyConstructor() const { return (!constructors.empty()); }

bool CoreType::hasAnyFromConvertor() const { return !fromConvertors.empty(); }

bool CoreType::hasConstructorWithTypes(Vec<IR::QatType *> types) const {
  for (auto *con : constructors) {
    auto argTys = con->getType()->asFunction()->getArgumentTypes();
    if (types.size() == (argTys.size() - 1)) {
      bool result = true;
      for (usize i = 1; i < argTys.size(); i++) {
        auto *argType = argTys.at(i)->getType();
        if ((!argType->isSame(types.at(i - 1))) &&
            (!(argType->isReference() &&
               argType->asReference()->getSubType()->isSame(
                   types.at(i - 1)))) &&
            (!(types.at(i - 1)->isReference() &&
               types.at(i - 1)->asReference()->getSubType()->isSame(
                   argType)))) {
          result = false;
          break;
        }
      }
      if (result) {
        return true;
      }
    } else {
      continue;
    }
  }
  return false;
}

MemberFunction *
CoreType::getConstructorWithTypes(Vec<IR::QatType *> types) const {
  for (auto *con : constructors) {
    auto argTys = con->getType()->asFunction()->getArgumentTypes();
    if (types.size() == (argTys.size() - 1)) {
      bool result = true;
      for (usize i = 1; i < argTys.size(); i++) {
        auto *argType = argTys.at(i)->getType();
        if ((!argType->isSame(types.at(i - 1))) &&
            (!(argType->isReference() &&
               argType->asReference()->getSubType()->isSame(
                   types.at(i - 1)))) &&
            (!(types.at(i - 1)->isReference() &&
               types.at(i - 1)->asReference()->getSubType()->isSame(
                   argType)))) {
          result = false;
          break;
        }
      }
      if (result) {
        return con;
      }
    } else {
      continue;
    }
  }
  return nullptr;
}

bool CoreType::hasFromConvertor(IR::QatType *typ) const {
  for (auto *fconv : fromConvertors) {
    auto *argTy =
        fconv->getType()->asFunction()->getArgumentTypeAt(1)->getType();
    if (argTy->isSame(typ) ||
        (argTy->isReference() &&
         argTy->asReference()->getSubType()->isSame(typ)) ||
        (typ->isReference() &&
         typ->asReference()->getSubType()->isSame(argTy))) {
      return true;
    }
  }
  return false;
}

MemberFunction *CoreType::getFromConvertor(IR::QatType *typ) const {
  for (auto *fconv : fromConvertors) {
    auto *argTy =
        fconv->getType()->asFunction()->getArgumentTypeAt(1)->getType();
    if (argTy->isSame(typ) ||
        (argTy->isReference() &&
         argTy->asReference()->getSubType()->isSame(typ)) ||
        (typ->isReference() &&
         typ->asReference()->getSubType()->isSame(argTy))) {
      return fconv;
    }
  }
  return nullptr;
}

bool CoreType::hasToConvertor(IR::QatType *typ) const {
  for (auto *fconv : fromConvertors) {
    auto *retTy = fconv->getType()->asFunction()->getReturnType();
    if (retTy->isSame(typ) ||
        (retTy->isReference() &&
         retTy->asReference()->getSubType()->isSame(typ)) ||
        (typ->isReference() &&
         typ->asReference()->getSubType()->isSame(retTy))) {
      return true;
    }
  }
  return false;
}

MemberFunction *CoreType::getToConvertor(IR::QatType *typ) const {
  for (auto *tconv : fromConvertors) {
    auto *retTy = tconv->getType()->asFunction()->getReturnType();
    if (retTy->isSame(typ) ||
        (retTy->isReference() &&
         retTy->asReference()->getSubType()->isSame(typ)) ||
        (typ->isReference() &&
         typ->asReference()->getSubType()->isSame(retTy))) {
      return tconv;
    }
  }
  return nullptr;
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

TemplateCoreType::TemplateCoreType(String                       _name,
                                   Vec<ast::TemplatedType *>    _templates,
                                   ast::DefineCoreType         *_defineCoreType,
                                   QatModule                   *_parent,
                                   const utils::VisibilityInfo &_visibInfo)
    : name(_name), templates(_templates), defineCoreType(_defineCoreType),
      parent(_parent), visibility(_visibInfo) {
  parent->templateCoreTypes.push_back(this);
}

String TemplateCoreType::getName() const { return name; }

utils::VisibilityInfo TemplateCoreType::getVisibility() const {
  return visibility;
}

usize TemplateCoreType::getTypeCount() const { return templates.size(); }

usize TemplateCoreType::getVariantCount() const { return variants.size(); }

QatModule *TemplateCoreType::getModule() const { return parent; }

CoreType *TemplateCoreType::fillTemplates(Vec<QatType *> types,
                                          IR::Context   *ctx) {
  for (auto var : variants) {
    if (var.check(types)) {
      return var.get();
    }
  }
  for (usize i = 0; i < templates.size(); i++) {
    templates.at(i)->setType(types.at(i));
  }
  (void)defineCoreType->define(ctx);
  (void)defineCoreType->emit(ctx);
  auto *cTy = (IR::CoreType *)defineCoreType->getCoreType();
  variants.push_back(TemplateVariant<CoreType>(cTy, types));
  for (auto *temp : templates) {
    temp->unsetType();
  }
  SHOW("Created variant for template core type: " << cTy->getFullName())
  return cTy;
}

} // namespace qat::IR