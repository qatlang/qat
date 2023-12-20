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

String ExpandedType::getFullName() const {
  auto result = parent->getFullNameWithChild(name.value);
  if (isGeneric()) {
    result += ":[";
    for (usize i = 0; i < generics.size(); i++) {
      result += generics.at(i)->toString();
      if (i != (generics.size() - 1)) {
        result += ", ";
      }
    }
    result += "]";
  }
  return result;
}

Identifier ExpandedType::getName() const { return name; }

Maybe<MemberFunction*> ExpandedType::checkNormalMemberFn(const Vec<MemberFunction*>& memberFunctions,
                                                         const String&               name) {
  for (auto* mFn : memberFunctions) {
    if (!mFn->isVariationFunction() && mFn->getName().value == name) {
      return mFn;
    }
  }
  return None;
}

bool ExpandedType::hasNormalMemberFn(const String& fnName) const {
  return checkNormalMemberFn(memberFunctions, fnName).has_value();
}

Maybe<MemberFunction*> ExpandedType::checkVariationFn(Vec<MemberFunction*> const& variationFunctions,
                                                      const String&               name) {
  for (auto* mFn : variationFunctions) {
    if (mFn->isVariationFunction() && mFn->getName().value == name) {
      return mFn;
    }
  }
  return None;
}

bool ExpandedType::hasVariationFn(String const& fnName) const {
  return checkVariationFn(memberFunctions, fnName).has_value();
}

MemberFunction* ExpandedType::getNormalMemberFn(const String& fnName) const {
  return checkNormalMemberFn(memberFunctions, fnName).value();
}

MemberFunction* ExpandedType::getVariationFn(const String& fnName) const {
  return checkVariationFn(memberFunctions, fnName).value();
}

Maybe<IR::MemberFunction*> ExpandedType::checkStaticFunction(Vec<IR::MemberFunction*> const& staticFns,
                                                             const String&                   name) {
  for (const auto& fun : staticFns) {
    if (fun->getName().value == name) {
      return fun;
    }
  }
  return None;
}

bool ExpandedType::hasStaticFunction(const String& fnName) const {
  return checkStaticFunction(staticFunctions, fnName).has_value();
}

MemberFunction* ExpandedType::getStaticFunction(const String& fnName) const {
  return checkStaticFunction(staticFunctions, fnName).value();
}

Maybe<IR::MemberFunction*> ExpandedType::checkBinaryOperator(Vec<IR::MemberFunction*> const& binOps, const String& opr,
                                                             Pair<Maybe<bool>, IR::QatType*> argType) {
  for (auto* bin : binOps) {
    if (bin->getName().value == opr) {
      auto* binArgTy = bin->getType()->asFunction()->getArgumentTypeAt(1)->getType();
      if (binArgTy->isSame(argType.second) ||
          (argType.first.has_value() && binArgTy->isReference() &&
           (binArgTy->asReference()->isSubtypeVariable() ? argType.first.value() : true) &&
           binArgTy->asReference()->getSubType()->isSame(argType.second)) ||
          (argType.second->isReference() && argType.second->asReference()->getSubType()->isSame(binArgTy))) {
        return bin;
      }
    }
  }
  return None;
}

bool ExpandedType::hasBinaryOperator(const String& opr, Pair<Maybe<bool>, IR::QatType*> argType) const {
  return checkBinaryOperator(binaryOperators, opr, argType).has_value();
}

MemberFunction* ExpandedType::getBinaryOperator(const String& opr, Pair<Maybe<bool>, IR::QatType*> argType) const {
  return checkBinaryOperator(binaryOperators, opr, argType).value();
}

Maybe<IR::MemberFunction*> ExpandedType::checkUnaryOperator(Vec<IR::MemberFunction*> const& unaryOperators,
                                                            const String&                   opr) {
  for (auto* unr : unaryOperators) {
    if (unr->getName().value == opr) {
      return unr;
    }
  }
  return None;
}

bool ExpandedType::hasUnaryOperator(const String& opr) const {
  return checkUnaryOperator(unaryOperators, opr).has_value();
}

MemberFunction* ExpandedType::getUnaryOperator(const String& opr) const {
  return checkUnaryOperator(unaryOperators, opr).value();
}

bool ExpandedType::hasDefaultConstructor() const { return defaultConstructor != nullptr; }

MemberFunction* ExpandedType::getDefaultConstructor() const { return defaultConstructor; }

bool ExpandedType::hasAnyConstructor() const { return (!constructors.empty()) || (defaultConstructor != nullptr); }

bool ExpandedType::hasAnyFromConvertor() const { return !fromConvertors.empty(); }

Maybe<IR::MemberFunction*> ExpandedType::checkConstructorWithTypes(Vec<IR::MemberFunction*> const&      cons,
                                                                   Vec<Pair<Maybe<bool>, IR::QatType*>> types) {
  for (auto* con : cons) {
    auto argTys = con->getType()->asFunction()->getArgumentTypes();
    if (types.size() == (argTys.size() - 1)) {
      bool result = true;
      for (usize i = 1; i < argTys.size(); i++) {
        auto* argType = argTys.at(i)->getType();
        if (!argType->isSame(types.at(i - 1).second) && !argType->isCompatible(types.at(i - 1).second) &&
            (!(types.at(i - 1).first.has_value() && argType->isReference() &&
               (argType->asReference()->isSubtypeVariable() ? types.at(i - 1).first.value() : true) &&
               (argType->asReference()->getSubType()->isSame(types.at(i - 1).second) ||
                argType->asReference()->getSubType()->isCompatible(types.at(i - 1).second)))) &&
            (!(types.at(i - 1).second->isReference() &&
               (types.at(i - 1).second->asReference()->getSubType()->isSame(argType) ||
                types.at(i - 1).second->asReference()->getSubType()->isCompatible(argType))))) {
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
  return None;
}

bool ExpandedType::hasConstructorWithTypes(Vec<Pair<Maybe<bool>, IR::QatType*>> argTypes) const {
  return checkConstructorWithTypes(constructors, argTypes).has_value();
}

MemberFunction* ExpandedType::getConstructorWithTypes(Vec<Pair<Maybe<bool>, IR::QatType*>> argTypes) const {
  return checkConstructorWithTypes(constructors, argTypes).value();
}

Maybe<IR::MemberFunction*> ExpandedType::checkFromConvertor(Vec<IR::MemberFunction*> const& fromConvs,
                                                            Maybe<bool> isValueVar, IR::QatType* candTy) {
  for (auto* fconv : fromConvs) {
    auto* argTy = fconv->getType()->asFunction()->getArgumentTypeAt(1)->getType();
    if (argTy->isSame(candTy) || argTy->isCompatible(candTy) ||
        (isValueVar.has_value() && argTy->isReference() &&
         (argTy->asReference()->isSubtypeVariable() ? isValueVar.value() : true) &&
         (argTy->asReference()->getSubType()->isSame(candTy) ||
          argTy->asReference()->getSubType()->isCompatible(candTy))) ||
        (candTy->isReference() && candTy->asReference()->getSubType()->isSame(argTy))) {
      return fconv;
    }
  }
  return None;
}

bool ExpandedType::hasFromConvertor(Maybe<bool> isValueVar, IR::QatType* candTy) const {
  return checkFromConvertor(fromConvertors, isValueVar, candTy).has_value();
}

MemberFunction* ExpandedType::getFromConvertor(Maybe<bool> isValueVar, IR::QatType* candTy) const {
  return checkFromConvertor(fromConvertors, isValueVar, candTy).value();
}

Maybe<MemberFunction*> ExpandedType::checkToConvertor(Vec<MemberFunction*> const& toConvertors, IR::QatType* targetTy) {
  for (auto* tconv : toConvertors) {
    auto* retTy = tconv->getType()->asFunction()->getReturnType()->getType();
    if (retTy->isSame(targetTy) || (retTy->isReference() && retTy->asReference()->getSubType()->isSame(targetTy)) ||
        (targetTy->isReference() && targetTy->asReference()->getSubType()->isSame(retTy))) {
      return tconv;
    }
  }
  return None;
}

bool ExpandedType::hasToConvertor(IR::QatType* typ) const {
  return ExpandedType::checkToConvertor(toConvertors, typ).has_value();
}

MemberFunction* ExpandedType::getToConvertor(IR::QatType* typ) const {
  return ExpandedType::checkToConvertor(toConvertors, typ).value();
}

bool ExpandedType::hasCopyConstructor() const { return copyConstructor.has_value(); }

MemberFunction* ExpandedType::getCopyConstructor() const { return copyConstructor.value(); }

bool ExpandedType::hasMoveConstructor() const { return moveConstructor.has_value(); }

MemberFunction* ExpandedType::getMoveConstructor() const { return moveConstructor.value(); }

bool ExpandedType::hasCopyAssignment() const { return copyAssignment.has_value(); }

MemberFunction* ExpandedType::getCopyAssignment() const { return copyAssignment.value(); }

bool ExpandedType::hasMoveAssignment() const { return moveAssignment.has_value(); }

MemberFunction* ExpandedType::getMoveAssignment() const { return moveAssignment.value(); }

bool ExpandedType::hasCopy() const { return hasCopyConstructor() && hasCopyAssignment(); }

bool ExpandedType::hasMove() const { return hasMoveConstructor() && hasMoveAssignment(); }

bool ExpandedType::hasDestructor() const { return destructor.has_value(); }

IR::MemberFunction* ExpandedType::getDestructor() const { return destructor.value(); }

VisibilityInfo ExpandedType::getVisibility() const { return visibility; }

bool ExpandedType::isAccessible(const AccessInfo& reqInfo) const { return visibility.isAccessible(reqInfo); }

QatModule* ExpandedType::getParent() { return parent; }

bool ExpandedType::isExpanded() const { return true; }

} // namespace qat::IR