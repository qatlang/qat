#include "./choice.hpp"
#include "../qat_module.hpp"
#include "llvm/IR/Constants.h"
#include <cmath>

namespace qat::IR {

#define MAX_PRERUN_GENERIC_BITWIDTH 64u

ChoiceType::ChoiceType(Identifier _name, QatModule* _parent, Vec<Identifier> _fields, Maybe<Vec<i64>> _values,
                       bool areValuesUnsigned, Maybe<usize> _defaultVal, const VisibilityInfo& _visibility,
                       llvm::LLVMContext& llctx, FileRange _fileRange)
    : EntityOverview("choiceType",
                     Json()
                         ._("moduleID", _parent->getID())
                         ._("hasValues", _values.has_value())
                         ._("hasDefault", _defaultVal.has_value())
                         ._("visibility", _visibility),
                     _name.range),
      name(std::move(_name)), parent(_parent), fields(std::move(_fields)), values(std::move(_values)),
      visibility(_visibility), defaultVal(_defaultVal), fileRange(std::move(_fileRange)) {
  if (!values.has_value()) {
    findBitwidthNormal();
    llvmType = llvm::Type::getIntNTy(llctx, bitwidth);
  } else {
    findBitwidthForValues();
    llvmType = llvm::Type::getIntNTy(llctx, bitwidth);
  }
  if (parent) {
    parent->choiceTypes.push_back(this);
  }
}

Identifier ChoiceType::getName() const { return name; }

String ChoiceType::getFullName() const { return parent->getFullNameWithChild(name.value); }

QatModule* ChoiceType::getParent() const { return parent; }

bool ChoiceType::hasNegativeValues() const { return !areValuesUnsigned; }

bool ChoiceType::hasCustomValue() const { return values.has_value(); }

bool ChoiceType::hasDefault() const { return defaultVal.has_value(); }

llvm::ConstantInt* ChoiceType::getDefault() const {
  return values.has_value() ? llvm::ConstantInt::get(llvm::Type::getIntNTy(llvmType->getContext(), bitwidth),
                                                     values->at(defaultVal.value()), !areValuesUnsigned)
                            : llvm::ConstantInt::get(llvm::Type::getIntNTy(llvmType->getContext(), bitwidth),
                                                     defaultVal.value(), false);
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
    return llvm::ConstantInt::get(llvm::Type::getIntNTy(llvmType->getContext(), bitwidth), values.value().at(index),
                                  !areValuesUnsigned);
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
      while (std::pow(2, result) < ((u64)val)) {
        result++;
      }
    } else if (std::pow(2, result) < val) {
      auto sigVal = val;
      if (std::pow(2, result) < (-sigVal)) {
        while (std::pow(2, result) < (-sigVal)) {
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
      valsValsJson.push_back((unsigned long long)val);
    }
  }
  ovInfo._("fields", fieldsJson)
      ._("values", valuesJson)
      ._("typeID", getID())
      ._("fullName", getFullName())
      ._("bitWidth", (unsigned long long)bitwidth)
      ._("areValuesUnsigned", areValuesUnsigned);
  if (defaultVal) {
    ovInfo._("defaultValue", (unsigned long long)defaultVal.value());
  }
}

bool ChoiceType::canBePrerunGeneric() const { return bitwidth <= MAX_PRERUN_GENERIC_BITWIDTH; }

Maybe<String> ChoiceType::toPrerunGenericString(IR::PrerunValue* val) const {
  if (canBePrerunGeneric()) {
    for (auto const& field : fields) {
      if (((u64)getValueFor(field.value)) ==
          *(llvm::cast<llvm::ConstantInt>(val->getLLVMConstant())->getValue().getRawData())) {
        return getFullName() + "'" + field.value;
      }
    }
    return None;
  } else {
    return None;
  }
}

bool ChoiceType::isTypeSized() const { return true; }

Maybe<bool> ChoiceType::equalityOf(IR::PrerunValue* first, IR::PrerunValue* second) const {
  if (first->getType()->isSame(second->getType())) {
    return (*llvm::cast<llvm::ConstantInt>(first->getLLVMConstant())->getValue().getRawData()) ==
           (*llvm::cast<llvm::ConstantInt>(second->getLLVMConstant())->getValue().getRawData());
  } else {
    return false;
  }
}

String ChoiceType::toString() const { return getFullName(); }

} // namespace qat::IR