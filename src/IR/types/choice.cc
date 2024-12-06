#include "./choice.hpp"
#include "../context.hpp"
#include "../qat_module.hpp"
#include "./c_type.hpp"
#include "./integer.hpp"
#include "./unsigned.hpp"

#include <cmath>
#include <llvm/Analysis/ConstantFolding.h>
#include <llvm/IR/ConstantFold.h>
#include <llvm/IR/Constants.h>

namespace qat::ir {

#define MAX_PRERUN_GENERIC_BITWIDTH 64u

ChoiceType::ChoiceType(Identifier _name, Mod* _parent, Vec<Identifier> _fields, Maybe<Vec<llvm::ConstantInt*>> _values,
                       Maybe<ir::Type*> _providedType, bool areValuesUnsigned, Maybe<usize> _defaultVal,
                       const VisibilityInfo& _visibility, ir::Ctx* _ctx, FileRange _fileRange,
                       Maybe<MetaInfo> _metaInfo)
    : EntityOverview("choiceType",
                     Json()
                         ._("moduleID", _parent->get_id())
                         ._("hasValues", _values.has_value())
                         ._("hasDefault", _defaultVal.has_value())
                         ._("visibility", _visibility),
                     _name.range),
      name(std::move(_name)), parent(_parent), fields(std::move(_fields)), values(std::move(_values)),
      providedType(_providedType), visibility(_visibility), defaultVal(_defaultVal), metaInfo(_metaInfo), irCtx(_ctx),
      fileRange(std::move(_fileRange)) {
  Maybe<String> foreignID;
  if (metaInfo) {
    foreignID = metaInfo->get_foreign_id();
  }
  linkingName = parent->get_link_names().newWith(LinkNameUnit(name.value, LinkUnitType::choice), foreignID).toName();
  if (values.has_value()) {
    if (providedType.has_value()) {
      llvmType = providedType.value()->get_llvm_type();
    } else {
      find_bitwidth_for_values();
      llvmType = llvm::Type::getIntNTy(irCtx->llctx, bitwidth);
    }
  } else {
    if (providedType.has_value()) {
      llvmType = providedType.value()->get_llvm_type();
    } else {
      find_bitwidth_normal();
      llvmType = llvm::Type::getIntNTy(irCtx->llctx, bitwidth);
    }
  }
  if (parent) {
    parent->choiceTypes.push_back(this);
  }
}

bool ChoiceType::has_provided_type() const { return providedType.has_value(); }

ir::Type* ChoiceType::get_provided_type() const { return providedType.value(); }

ir::Type* ChoiceType::get_underlying_type() const {
  return providedType.has_value() ? providedType.value()
                                  : (has_negative_values() ? (ir::Type*)ir::IntegerType::get(bitwidth, irCtx)
                                                           : ir::UnsignedType::create(bitwidth, irCtx));
}

Identifier ChoiceType::get_name() const { return name; }

String ChoiceType::get_full_name() const { return parent->get_fullname_with_child(name.value); }

Mod* ChoiceType::get_module() const { return parent; }

bool ChoiceType::has_negative_values() const {
  return (!areValuesUnsigned) ||
         (providedType.has_value() &&
          (providedType.value()->is_integer() ||
           (providedType.value()->is_ctype() && providedType.value()->as_ctype()->get_subtype()->is_integer())));
}

bool ChoiceType::has_custom_value() const { return values.has_value(); }

bool ChoiceType::has_default() const { return defaultVal.has_value(); }

llvm::ConstantInt* ChoiceType::get_default() const {
  return values.has_value() ? values->at(defaultVal.value())
                            : llvm::cast<llvm::ConstantInt>(llvm::ConstantInt::get(
                                  llvmType, defaultVal.value(),
                                  providedType.has_value() ? providedType.value()->is_integer() : false));
}

bool ChoiceType::has_field(const String& name) const {
  for (const auto& field : fields) {
    if (field.value == name) {
      return true;
    }
  }
  return false;
}

llvm::ConstantInt* ChoiceType::get_value_for(const String& name) const {
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

void ChoiceType::find_bitwidth_normal() const {
  auto calc = 1;
  while (std::pow(2, calc) < fields.size()) {
    calc <<= 1;
    bitwidth++;
  }
}

void ChoiceType::find_bitwidth_for_values() const {
  u64 result = 1;
  for (auto val : values.value()) {
    if (areValuesUnsigned) {
      while (llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldCompareInstruction(
                                               llvm::CmpInst::Predicate::ICMP_ULT,
                                               llvm::ConstantInt::get(val->getType(), std::pow(2, result), false), val))
                 ->getUniqueInteger()
                 .getBoolValue()) {
        result++;
      }
    } else if (llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldCompareInstruction(
                                                 llvm::CmpInst::Predicate::ICMP_SLT,
                                                 llvm::ConstantInt::get(val->getType(), std::pow(2, result), true),
                                                 val))
                   ->getUniqueInteger()
                   .getBoolValue()) {
      auto sigVal = llvm::ConstantExpr::getNeg(val);
      if (llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldCompareInstruction(
                                            llvm::CmpInst::Predicate::ICMP_SLT,
                                            llvm::ConstantInt::get(val->getType(), std::pow(2, result), true), sigVal))
              ->getUniqueInteger()
              .getBoolValue()) {
        while (llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldCompareInstruction(
                                                 llvm::CmpInst::Predicate::ICMP_SLT,
                                                 llvm::ConstantInt::get(val->getType(), std::pow(2, result), true),
                                                 sigVal))
                   ->getUniqueInteger()
                   .getBoolValue()) {
          result++;
        }
      }
    }
  }
  bitwidth = areValuesUnsigned ? result : (result + 1);
}

void ChoiceType::get_missing_names(Vec<Identifier>& vals, Vec<Identifier>& missing) const {
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

const VisibilityInfo& ChoiceType::get_visibility() const { return visibility; }

void ChoiceType::update_overview() {
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
      ._("typeID", get_id())
      ._("fullName", get_full_name())
      ._("bitWidth", bitwidth)
      ._("areValuesUnsigned", areValuesUnsigned);
  if (defaultVal) {
    ovInfo._("defaultValue", defaultVal.value());
  }
}

Maybe<String> ChoiceType::to_prerun_generic_string(ir::PrerunValue* val) const {
  if (can_be_prerun_generic()) {
    for (auto const& field : fields) {
      if (llvm::cast<llvm::ConstantInt>(
              llvm::ConstantFoldCompareInstruction(llvm::CmpInst::Predicate::ICMP_EQ, get_value_for(field.value),
                                                   llvm::cast<llvm::ConstantInt>(val->get_llvm_constant())))
              ->getValue()
              .getBoolValue()) {
        return get_full_name() + "::" + field.value;
      }
    }
    return None;
  } else {
    return None;
  }
}

bool ChoiceType::is_type_sized() const { return true; }

Maybe<bool> ChoiceType::equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const {
  if (first->get_ir_type()->is_same(second->get_ir_type())) {
    return llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldCompareInstruction(llvm::CmpInst::ICMP_EQ,
                                                                              first->get_llvm_constant(),
                                                                              second->get_llvm_constant()))
        ->getValue()
        .getBoolValue();
  } else {
    return false;
  }
}

String ChoiceType::to_string() const { return get_full_name(); }

} // namespace qat::ir
