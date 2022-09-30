#include "./choice.hpp"
#include "../qat_module.hpp"

namespace qat::IR {

ChoiceType::ChoiceType(String _name, QatModule *_parent, Vec<String> _fields,
                       Maybe<Vec<i64>> _values, Maybe<usize> _defaultVal,
                       const utils::VisibilityInfo &_visibility,
                       llvm::LLVMContext           &ctx)
    : name(std::move(_name)), parent(_parent), fields(std::move(_fields)),
      values(std::move(_values)), visibility(_visibility),
      defaultVal(_defaultVal) {
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

String ChoiceType::getName() const { return name; }

String ChoiceType::getFullName() const {
  return parent->getFullNameWithChild(name);
}

QatModule *ChoiceType::getParent() const { return parent; }

bool ChoiceType::hasCustomValue() const { return values.has_value(); }

bool ChoiceType::hasDefault() const { return defaultVal.has_value(); }

i64 ChoiceType::getDefault() const {
  return values.has_value() ? values->at(defaultVal.value())
                            : (i64)defaultVal.value();
}

bool ChoiceType::hasField(const String &name) const {
  for (const auto &field : fields) {
    if (field == name) {
      return true;
    }
  }
  return false;
}

i64 ChoiceType::getValueFor(const String &name) const {
  usize index = 0;
  for (usize i = 0; i < fields.size(); i++) {
    if (fields.at(i) == name) {
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
  for (const auto &val : values.value()) {
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

void ChoiceType::getMissingNames(Vec<String> &vals,
                                 Vec<String> &missing) const {
  for (const auto &sub : fields) {
    bool result = false;
    for (const auto &val : vals) {
      if (sub == val) {
        result = true;
        break;
      }
    }
    if (!result) {
      missing.push_back(sub);
    }
  }
}

const utils::VisibilityInfo &ChoiceType::getVisibility() const {
  return visibility;
}

} // namespace qat::IR