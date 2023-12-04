#include "./expanded_type.hpp"
#include "../../show.hpp"
#include "../../utils/split_string.hpp"
#include "../context.hpp"
#include "../generics.hpp"
#include "../qat_module.hpp"
#include "./function.hpp"
#include "./reference.hpp"

namespace qat::IR {

ExpandedType::ExpandedType(Identifier _name, Vec<GenericParameter*> _generics, IR::QatModule* _parent,
                           const VisibilityInfo& _visib)
    : name(std::move(_name)), generics(_generics), parent(_parent), visibility(_visib) {}

bool ExpandedType::isGeneric() const { return !generics.empty(); }

bool ExpandedType::hasGenericParameter(const String& name) const {
  for (auto* gen : generics) {
    if (gen->isSame(name)) {
      return true;
    }
  }
  return false;
}

GenericParameter* ExpandedType::getGenericParameter(const String& name) const {
  for (auto* gen : generics) {
    if (gen->isSame(name)) {
      return gen;
    }
  }
  return nullptr;
}

String ExpandedType::getFullName() const { return parent->getFullNameWithChild(name.value); }

Identifier ExpandedType::getName() const { return name; }

bool ExpandedType::hasMemberFunction(const String& fnName) const {
  for (auto* memberFunction : memberFunctions) {
    SHOW("Found member function: " << memberFunction->getName().value)
    if (memberFunction->getName().value == fnName) {
      return true;
    }
  }
  return false;
}

MemberFunction* ExpandedType::getMemberFunction(const String& fnName) const {
  for (auto* memberFunction : memberFunctions) {
    if (memberFunction->getName().value == fnName) {
      return memberFunction;
    }
  }
  return nullptr;
}

bool ExpandedType::hasStaticFunction(const String& fnName) const {
  for (const auto& fun : staticFunctions) {
    if (fun->getName().value == fnName) {
      return true;
    }
  }
  return false;
}

MemberFunction* ExpandedType::getStaticFunction(const String& fnName) const {
  for (auto* staticFunction : staticFunctions) {
    if (staticFunction->getName().value == fnName) {
      return staticFunction;
    }
  }
  return nullptr;
}

bool ExpandedType::hasBinaryOperator(const String& opr, IR::QatType* type) const {
  for (auto* bin : binaryOperators) {
    if (utils::splitString(bin->getName().value, "'")[1] == opr) {
      auto* binArgTy = bin->getType()->asFunction()->getArgumentTypeAt(1)->getType();
      if (binArgTy->isSame(type) || (binArgTy->isReference() && binArgTy->asReference()->getSubType()->isSame(type))) {
        return true;
      }
    }
  }
  return false;
}

MemberFunction* ExpandedType::getBinaryOperator(const String& opr, IR::QatType* type) const {
  for (auto* bin : binaryOperators) {
    if (utils::splitString(bin->getName().value, "'")[1] == opr) {
      auto* binArgTy = bin->getType()->asFunction()->getArgumentTypeAt(1)->getType();
      if (binArgTy->isSame(type) || (binArgTy->isReference() && binArgTy->asReference()->getSubType()->isSame(type))) {
        return bin;
      }
    }
  }
  return nullptr;
}

bool ExpandedType::hasUnaryOperator(const String& opr) const {
  for (auto* unr : unaryOperators) {
    if (utils::splitString(unr->getName().value, "'")[1] == opr) {
      return true;
    }
  }
  return false;
}

MemberFunction* ExpandedType::getUnaryOperator(const String& opr) const {
  for (auto* unr : unaryOperators) {
    if (utils::splitString(unr->getName().value, "'")[1] == opr) {
      return unr;
    }
  }
  return nullptr;
}

u64 ExpandedType::getOperatorVariantIndex(const String& opr) const {
  u64 index = 0;
  for (auto* bin : binaryOperators) {
    if (utils::splitString(bin->getName().value, "'")[1] == opr) {
      index++;
    }
  }
  return index;
}

bool ExpandedType::hasDefaultConstructor() const { return defaultConstructor != nullptr; }

MemberFunction* ExpandedType::getDefaultConstructor() const { return defaultConstructor; }

bool ExpandedType::hasAnyConstructor() const { return (!constructors.empty()) || (defaultConstructor != nullptr); }

bool ExpandedType::hasAnyFromConvertor() const { return !fromConvertors.empty(); }

bool ExpandedType::hasConstructorWithTypes(Vec<IR::QatType*> types) const {
  for (auto* con : constructors) {
    auto argTys = con->getType()->asFunction()->getArgumentTypes();
    if (types.size() == (argTys.size() - 1)) {
      bool result = true;
      for (usize i = 1; i < argTys.size(); i++) {
        auto* argType = argTys.at(i)->getType();
        if (!argType->isSame(types.at(i - 1)) && !argType->isCompatible(types.at(i - 1)) &&
            (!(argType->isReference() && (argType->asReference()->getSubType()->isSame(types.at(i - 1)) ||
                                          argType->asReference()->getSubType()->isCompatible(types.at(i - 1))))) &&
            (!(types.at(i - 1)->isReference() &&
               (types.at(i - 1)->asReference()->getSubType()->isSame(argType) ||
                types.at(i - 1)->asReference()->getSubType()->isCompatible(argType))))) {
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

MemberFunction* ExpandedType::getConstructorWithTypes(Vec<IR::QatType*> types) const {
  for (auto* con : constructors) {
    auto argTys = con->getType()->asFunction()->getArgumentTypes();
    if (types.size() == (argTys.size() - 1)) {
      bool result = true;
      for (usize i = 1; i < argTys.size(); i++) {
        auto* argType = argTys.at(i)->getType();
        if (!argType->isSame(types.at(i - 1)) && !argType->isCompatible(types.at(i - 1)) &&
            (!(argType->isReference() && (argType->asReference()->getSubType()->isSame(types.at(i - 1)) ||
                                          argType->asReference()->getSubType()->isCompatible(types.at(i - 1))))) &&
            (!(types.at(i - 1)->isReference() && types.at(i - 1)->asReference()->getSubType()->isSame(argType)))) {
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

bool ExpandedType::hasFromConvertor(IR::QatType* typ) const {
  for (auto* fconv : fromConvertors) {
    auto* argTy = fconv->getType()->asFunction()->getArgumentTypeAt(1)->getType();
    if (argTy->isSame(typ) || argTy->isCompatible(typ) ||
        (argTy->isReference() &&
         (argTy->asReference()->getSubType()->isSame(typ) || argTy->asReference()->getSubType()->isCompatible(typ))) ||
        (typ->isReference() && typ->asReference()->getSubType()->isSame(argTy))) {
      return true;
    }
  }
  return false;
}

MemberFunction* ExpandedType::getFromConvertor(IR::QatType* typ) const {
  for (auto* fconv : fromConvertors) {
    auto* argTy = fconv->getType()->asFunction()->getArgumentTypeAt(1)->getType();
    if (argTy->isSame(typ) || argTy->isCompatible(typ) ||
        (argTy->isReference() &&
         (argTy->asReference()->getSubType()->isSame(typ) || argTy->asReference()->getSubType()->isCompatible(typ))) ||
        (typ->isReference() && typ->asReference()->getSubType()->isSame(argTy))) {
      return fconv;
    }
  }
  return nullptr;
}

bool ExpandedType::hasToConvertor(IR::QatType* typ) const {
  for (auto* fconv : fromConvertors) {
    auto* retTy = fconv->getType()->asFunction()->getReturnType();
    if (retTy->isSame(typ) || (retTy->isReference() && retTy->asReference()->getSubType()->isSame(typ)) ||
        (typ->isReference() && typ->asReference()->getSubType()->isSame(retTy))) {
      return true;
    }
  }
  return false;
}

MemberFunction* ExpandedType::getToConvertor(IR::QatType* typ) const {
  for (auto* tconv : fromConvertors) {
    auto* retTy = tconv->getType()->asFunction()->getReturnType();
    if (retTy->isSame(typ) || (retTy->isReference() && retTy->asReference()->getSubType()->isSame(typ)) ||
        (typ->isReference() && typ->asReference()->getSubType()->isSame(retTy))) {
      return tconv;
    }
  }
  return nullptr;
}

bool ExpandedType::hasCopyConstructor() const { return copyConstructor.has_value(); }

MemberFunction* ExpandedType::getCopyConstructor() const { return copyConstructor.value_or(nullptr); }

bool ExpandedType::hasMoveConstructor() const { return moveConstructor.has_value(); }

MemberFunction* ExpandedType::getMoveConstructor() const { return moveConstructor.value_or(nullptr); }

bool ExpandedType::hasCopyAssignment() const { return copyAssignment.has_value(); }

MemberFunction* ExpandedType::getCopyAssignment() const { return copyAssignment.value_or(nullptr); }

bool ExpandedType::hasMoveAssignment() const { return moveAssignment.has_value(); }

MemberFunction* ExpandedType::getMoveAssignment() const { return moveAssignment.value_or(nullptr); }

bool ExpandedType::hasCopy() const { return hasCopyConstructor() && hasCopyAssignment(); }

bool ExpandedType::hasMove() const { return hasMoveConstructor() && hasMoveAssignment(); }

bool ExpandedType::hasDestructor() const { return destructor != nullptr; }

void ExpandedType::createDestructor(FileRange fRange, IR::Context* ctx) {
  if (!destructor.has_value()) {
    destructor = IR::MemberFunction::CreateDestructor(this, nullptr, fRange, fRange, ctx);
  }
}

IR::MemberFunction* ExpandedType::getDestructor() const { return destructor.value(); }

VisibilityInfo ExpandedType::getVisibility() const { return visibility; }

bool ExpandedType::isAccessible(const AccessInfo& reqInfo) const { return visibility.isAccessible(reqInfo); }

QatModule* ExpandedType::getParent() { return parent; }

bool ExpandedType::isExpanded() const { return true; }

bool ExpandedType::isDestructible() const {
  SHOW("ExpandedType isDestructible: " << ((hasDefinedDestructor || needsImplicitDestructor || (destructor != nullptr))
                                               ? "true"
                                               : "false"))
  return hasDefinedDestructor || needsImplicitDestructor || (destructor != nullptr);
}

void ExpandedType::destroyValue(IR::Context* ctx, Vec<IR::Value*> vals, IR::Function* fun) {
  if (destructor.has_value() && !vals.empty()) {
    auto* inst = vals.front();
    if (inst->isReference()) {
      inst->loadImplicitPointer(ctx->builder);
    }
    (void)destructor.value()->call(ctx, {inst->getLLVM()}, ctx->getMod());
  }
}

} // namespace qat::IR