#include "./choice.hpp"
#include "../qat_module.hpp"

namespace qat::IR {

#define MAX_CONST_GENERIC_BITWIDTH 64u

ChoiceType::ChoiceType(Identifier _name, QatModule* _parent, Vec<Identifier> _fields, Maybe<Vec<i64>> _values,
                       Maybe<usize> _defaultVal, const VisibilityInfo& _visibility, llvm::LLVMContext& ctx,
                       FileRange _fileRange)
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
    llvmType = llvm::Type::getIntNTy(ctx, bitwidth);
  } else {
    findBitwidthForValues();
    llvmType = llvm::Type::getIntNTy(ctx, bitwidth);
  }
  if (parent) {
    parent->choiceTypes.push_back(this);
  }
}

Identifier ChoiceType::getName() const { return name; }

String ChoiceType::getFullName() const { return parent->getFullNameWithChild(name.value); }

QatModule* ChoiceType::getParent() const { return parent; }

bool ChoiceType::hasNegativeValues() const { return hasNegative; }

bool ChoiceType::hasCustomValue() const { return values.has_value(); }

bool ChoiceType::hasDefault() const { return defaultVal.has_value(); }

i64 ChoiceType::getDefault() const {
  return values.has_value() ? values->at(defaultVal.value()) : (i64)defaultVal.value();
}

bool ChoiceType::hasField(const String& name) const {
  for (const auto& field : fields) {
    if (field.value == name) {
      return true;
    }
  }
  return false;
}

i64 ChoiceType::getValueFor(const String& name) const {
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
    return index;
  }
}

u64 ChoiceType::getBitwidth() const { return bitwidth; }

void ChoiceType::findBitwidthNormal() const {
  auto calc = 2;
  while (calc <= fields.size()) {
    calc <<= 1;
    bitwidth++;
  }
}

void ChoiceType::findBitwidthForValues() const {
  u64 result = 1;
  i64 calc   = 2;
  for (const auto& val : values.value()) {
    if (val < 0) {
      hasNegative = true;
      if (calc < (-val)) {
        while (calc < (-val)) {
          calc *= 2;
          result++;
        }
      }
    } else if (calc < val) {
      hasNegative = false;
      while (calc < val) {
        calc *= 2;
        result++;
      }
    }
  }
  bitwidth = hasNegative ? (result + 1) : result;
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
      ._("hasNegative", hasNegative);
  if (defaultVal) {
    ovInfo._("defaultValue", (unsigned long long)defaultVal.value());
  }
}

bool ChoiceType::canBeConstGeneric() const { return bitwidth <= MAX_CONST_GENERIC_BITWIDTH; }

Maybe<String> ChoiceType::toConstGenericString(IR::ConstantValue* val) const {
  if (canBeConstGeneric()) {
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

Maybe<bool> ChoiceType::equalityOf(IR::ConstantValue* first, IR::ConstantValue* second) const {
  if (first->getType()->isSame(second->getType())) {
    return (*llvm::cast<llvm::ConstantInt>(first->getLLVMConstant())->getValue().getRawData()) ==
           (*llvm::cast<llvm::ConstantInt>(second->getLLVMConstant())->getValue().getRawData());
  } else {
    return false;
  }
}

String ChoiceType::toString() const { return getFullName(); }

} // namespace qat::IR