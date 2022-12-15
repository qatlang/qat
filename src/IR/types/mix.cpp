#include "./mix.hpp"
#include "../../show.hpp"
#include "../qat_module.hpp"
#include "type_kind.hpp"
#include "llvm/IR/LLVMContext.h"
#include <algorithm>

namespace qat::IR {

#define TWO_POWER_4  16ULL
#define TWO_POWER_8  256ULL
#define TWO_POWER_16 65536ULL
#define TWO_POWER_32 4294967296ULL

MixType::MixType(Identifier _name, QatModule* _parent, Vec<Pair<Identifier, Maybe<QatType*>>> _subtypes,
                 Maybe<usize> _defaultVal, llvm::LLVMContext& ctx, bool _isPacked,
                 const utils::VisibilityInfo& _visibility, FileRange _fileRange)
    : EntityOverview("mixType",
                     Json()
                         ._("moduleID", _parent->getID())
                         ._("hasDefaultValue", _defaultVal.has_value())
                         ._("visibility", _visibility),
                     _name.range),
      name(std::move(_name)), parent(_parent), subtypes(std::move(_subtypes)), isPack(_isPacked),
      visibility(_visibility), defaultVal(_defaultVal), fileRange(std::move(_fileRange)) {
  for (const auto& sub : subtypes) {
    if (sub.second.has_value()) {
      auto* typ = sub.second.value();
      SHOW("Getting size of the subtype in SUM TYPE")
      auto* llSize =
          llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx),
                                 parent->getLLVMModule()->getDataLayout().getTypeStoreSizeInBits(typ->getLLVMType()));
      SHOW("Size query type ID: " << llSize->getType()->getTypeID())
      auto size = *(llSize->getValue().getRawData());
      SHOW("Got size " << size << " of subtype named " << sub.first.value)
      if (size > maxSize) {
        maxSize = size;
      }
    }
  }
  findTagBitWidth(subtypes.size());
  SHOW("Tag bitwidth is " << tagBitWidth)
  llvmType =
      llvm::StructType::create(ctx, {llvm::Type::getIntNTy(ctx, tagBitWidth), llvm::Type::getIntNTy(ctx, maxSize)},
                               parent->getFullNameWithChild(name.value), _isPacked);
  if (parent) {
    parent->mixTypes.push_back(this);
  }
}

void MixType::updateOverview() {
  Vec<JsonValue> subTyJson;
  for (auto const& sub : subtypes) {
    subTyJson.push_back(Json()
                            ._("name", sub.first)
                            ._("hasType", sub.second.has_value())
                            ._("typeID", sub.second.has_value() ? sub.second.value()->getID() : JsonValue())
                            ._("type", sub.second.has_value() ? sub.second.value()->toString() : JsonValue()));
  }
  ovInfo._("typeID", getID())
      ._("fullName", getFullName())
      ._("tagBitWidth", (unsigned long long)tagBitWidth)
      ._("isPacked", isPack)
      ._("subTypes", subTyJson);
  if (hasDefault()) {
    ovInfo._("defaultValue", (unsigned long long)defaultVal.value());
  }
}

void MixType::findTagBitWidth(usize typeCount) {
  auto calc = 2;
  while (calc <= typeCount) {
    calc <<= 1;
    tagBitWidth++;
  }
}

Identifier MixType::getName() const { return name; }

String MixType::getFullName() const { return parent->getFullNameWithChild(name.value); }

usize MixType::getIndexOfName(const String& name) const {
  for (usize i = 0; i < subtypes.size(); i++) {
    if (subtypes.at(i).first.value == name) {
      return i;
    }
  }
  return subtypes.size();
}

bool MixType::hasDefault() const { return defaultVal.has_value(); }

usize MixType::getDefault() const { return defaultVal.value_or(0u); }

Pair<bool, bool> MixType::hasSubTypeWithName(const String& sname) const {
  for (const auto& sTy : subtypes) {
    if (sTy.first.value == sname) {
      return {true, sTy.second.has_value()};
    }
  }
  return {false, false};
}

QatType* MixType::getSubTypeWithName(const String& sname) const {
  for (const auto& sTy : subtypes) {
    if (sTy.first.value == sname) {
      return sTy.second.value();
    }
  }
  return nullptr;
}

void MixType::getMissingNames(Vec<Identifier>& vals, Vec<Identifier>& missing) const {
  for (const auto& sub : subtypes) {
    bool result = false;
    for (const auto& val : vals) {
      if (sub.first.value == val.value) {
        result = true;
        break;
      }
    }
    if (!result) {
      missing.push_back(sub.first);
    }
  }
}

usize MixType::getSubTypeCount() const { return subtypes.size(); }

QatModule* MixType::getParent() const { return parent; }

bool MixType::isPacked() const { return isPack; }

usize MixType::getTagBitwidth() const { return tagBitWidth; }

u64 MixType::getDataBitwidth() const { return maxSize; }

bool MixType::isAccessible(const utils::RequesterInfo& reqInfo) const { return visibility.isAccessible(reqInfo); }

const utils::VisibilityInfo& MixType::getVisibility() const { return visibility; }

FileRange MixType::getFileRange() const { return fileRange; }

String MixType::toString() const { return getFullName(); }

TypeKind MixType::typeKind() const { return TypeKind::mixType; }

} // namespace qat::IR