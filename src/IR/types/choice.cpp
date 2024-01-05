#include "./choice.hpp"
#include "../context.hpp"
#include "../qat_module.hpp"
#include "llvm/Analysis/ConstantFolding.h"
#include "llvm/IR/Constants.h"
#include <cmath>

namespace qat::IR {

#define MAX_PRERUN_GENERIC_BITWIDTH 64u

ChoiceType::ChoiceType(Identifier _name, QatModule* _parent, Vec<Identifier> _fields,
                       Maybe<Vec<llvm::ConstantInt*>> _values, Maybe<IR::QatType*> _providedType,
                       bool areValuesUnsigned, Maybe<usize> _defaultVal, const VisibilityInfo& _visibility,
                       IR::Context* _ctx, FileRange _fileRange, Maybe<MetaInfo> _metaInfo)
    : EntityOverview("choiceType",
                     Json()
                         ._("moduleID", _parent->getID())
                         ._("hasValues", _values.has_value())
                         ._("hasDefault", _defaultVal.has_value())
                         ._("visibility", _visibility),
                     _name.range),
      name(std::move(_name)), parent(_parent), fields(std::move(_fields)), values(std::move(_values)),
      providedType(_providedType), visibility(_visibility), defaultVal(_defaultVal), metaInfo(_metaInfo), ctx(_ctx),
      fileRange(std::move(_fileRange)) {
  Maybe<String> foreignID;
  if (metaInfo) {
    foreignID = metaInfo->getForeignID();
  }
  linkingName = parent->getLinkNames().newWith(LinkNameUnit(name.value, LinkUnitType::choice), foreignID).toName();
  if (values.has_value()) {
    if (providedType.has_value()) {
      llvmType = providedType.value()->getLLVMType();
    } else {
      findBitwidthForValues();
      llvmType = llvm::Type::getIntNTy(ctx->llctx, bitwidth);
    }
  } else {
    if (providedType.has_value()) {
      llvmType = providedType.value()->getLLVMType();
    } else {
      findBitwidthNormal();
      llvmType = llvm::Type::getIntNTy(ctx->llctx, bitwidth);
    }
  }
  if (parent) {
    parent->choiceTypes.push_back(this);
  }
}

bool ChoiceType::hasProvidedType() const { return providedType.has_value(); }

IR::QatType* ChoiceType::getProvidedType() const { return providedType.value(); }

Identifier ChoiceType::getName() const { return name; }

String ChoiceType::getFullName() const { return parent->getFullNameWithChild(name.value); }

QatModule* ChoiceType::getParent() const { return parent; }

bool ChoiceType::hasNegativeValues() const { return !areValuesUnsigned; }

bool ChoiceType::hasCustomValue() const { return values.has_value(); }

bool ChoiceType::hasDefault() const { return defaultVal.has_value(); }

llvm::ConstantInt* ChoiceType::getDefault() const {
  return values.has_value()
             ? values->at(defaultVal.value())
             : llvm::cast<llvm::ConstantInt>(llvm::ConstantInt::get(
                   llvmType, defaultVal.value(), providedType.has_value() ? providedType.value()->isInteger() : false));
}

bool ChoiceType::hasField(const String& name) const {
  for (const auto& field : fields) {
    if (field.value == name) {
      return true;
    }
  }
  return false;
}

llvm::ConstantInt* ChoiceType::getValueFor(const String& name) const {
  usize index = 0;
  for (usize i = 0; i < fields.size(); i++) {
    if (fields.at(i).value == name) {
      index = i;
      break;
    }
  }
  if (values.has_value()) {
    return values.value().at(index);
  } else {
    return llvm::ConstantInt::get(llvm::Type::getIntNTy(llvmType->getContext(), bitwidth), index, false);
  }
}

u64 ChoiceType::getBitwidth() const { return bitwidth; }

void ChoiceType::findBitwidthNormal() const {
  auto calc = 1;
  while (std::pow(2, calc) < fields.size()) {
    calc <<= 1;
    bitwidth++;
  }
}

void ChoiceType::findBitwidthForValues() const {
  u64 result = 1;
  for (auto val : values.value()) {
    if (areValuesUnsigned) {
      while (
          llvm::cast<llvm::ConstantInt>(
              llvm::ConstantFoldConstant(
                  llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_ULT,
                                              llvm::ConstantInt::get(val->getType(), std::pow(2, result), false), val),
                  ctx->dataLayout.value()))
              ->getUniqueInteger()
              .getBoolValue()) {
        result++;
      }
    } else if (llvm::cast<llvm::ConstantInt>(
                   llvm::ConstantFoldConstant(
                       llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_SLT,
                                                   llvm::ConstantInt::get(val->getType(), std::pow(2, result), true),
                                                   val),
                       ctx->dataLayout.value()))
                   ->getUniqueInteger()
                   .getBoolValue()) {
      auto sigVal = llvm::ConstantExpr::getNeg(val);
      if (llvm::cast<llvm::ConstantInt>(
              llvm::ConstantFoldConstant(llvm::ConstantExpr::getICmp(
                                             llvm::CmpInst::Predicate::ICMP_SLT,
                                             llvm::ConstantInt::get(val->getType(), std::pow(2, result), true), sigVal),
                                         ctx->dataLayout.value()))
              ->getUniqueInteger()
              .getBoolValue()) {
        while (llvm::cast<llvm::ConstantInt>(
                   llvm::ConstantFoldConstant(
                       llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_SLT,
                                                   llvm::ConstantInt::get(val->getType(), std::pow(2, result), true),
                                                   sigVal),
                       ctx->dataLayout.value()))
                   ->getUniqueInteger()
                   .getBoolValue()) {
          result++;
        }
      }
    }
  }
  bitwidth = areValuesUnsigned ? result : (result + 1);
}

void ChoiceType::getMissingNames(Vec<Identifier>& vals, Vec<Identifier>& missing) const {
  for (const auto& sub : fields) {
    bool result = false;
    for (const auto& val : vals) {
      if (sub.value == val.value) {
        result = true;
        break;
      }
    }
    if (!result) {
      missing.push_back(sub);
    }
  }
}

const VisibilityInfo& ChoiceType::getVisibility() const { return visibility; }

void ChoiceType::updateOverview() {
  Vec<JsonValue> fieldsJson;
  for (const auto& field : fields) {
    fieldsJson.push_back(field);
  }
  auto valuesJson = Json();
  if (values) {
    Vec<JsonValue> valsValsJson;
    for (auto val : values.value()) {
      valsValsJson.push_back(val);
    }
  }
  ovInfo._("fields", fieldsJson)
      ._("values", valuesJson)
      ._("typeID", getID())
      ._("fullName", getFullName())
      ._("bitWidth", bitwidth)
      ._("areValuesUnsigned", areValuesUnsigned);
  if (defaultVal) {
    ovInfo._("defaultValue", defaultVal.value());
  }
}

bool ChoiceType::canBePrerunGeneric() const { return true; }

Maybe<String> ChoiceType::toPrerunGenericString(IR::PrerunValue* val) const {
  if (canBePrerunGeneric()) {
    for (auto const& field : fields) {
      if (llvm::cast<llvm::ConstantInt>(
              llvm::ConstantFoldConstant(
                  llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_EQ, getValueFor(field.value),
                                              llvm::cast<llvm::ConstantInt>(val->getLLVMConstant())),
                  ctx->dataLayout.value()))
              ->getValue()
              .getBoolValue()) {
        return getFullName() + "::" + field.value;
      }
    }
    return None;
  } else {
    return None;
  }
}

bool ChoiceType::isTypeSized() const { return true; }

Maybe<bool> ChoiceType::equalityOf(IR::Context* ctx, IR::PrerunValue* first, IR::PrerunValue* second) const {
  if (first->getType()->isSame(second->getType())) {
    return llvm::cast<llvm::ConstantInt>(
               llvm::ConstantFoldConstant(llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_EQ, first->getLLVMConstant(),
                                                                      second->getLLVMConstant()),
                                          ctx->dataLayout.value()))
        ->getValue()
        .getBoolValue();
  } else {
    return false;
  }
}

String ChoiceType::toString() const { return getFullName(); }

} // namespace qat::IR