#include "./union.hpp"
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

UnionType::UnionType(String _name, QatModule *_parent,
                     Vec<Pair<String, Maybe<QatType *>>> _subtypes,
                     llvm::LLVMContext &ctx, bool _isPacked,
                     const utils::VisibilityInfo &_visibility)
    : name(std::move(_name)), parent(_parent), subtypes(std::move(_subtypes)),
      isPack(_isPacked), visibility(_visibility) {
  for (const auto &sub : subtypes) {
    if (sub.second.has_value()) {
      auto *typ = sub.second.value();
      SHOW("Getting size of the subtype in SUM TYPE")
      auto *llSize = llvm::ConstantInt::get(
          llvm::Type::getInt64Ty(ctx),
          parent->getLLVMModule()->getDataLayout().getTypeStoreSizeInBits(
              typ->getLLVMType()));
      SHOW("Size query type ID: " << llSize->getType()->getTypeID())
      auto size = *(llSize->getValue().getRawData());
      SHOW("Got size " << size << " of subtype named " << sub.first)
      if (size > maxSize) {
        maxSize = size;
      }
    }
  }
  findTagBitWidth(subtypes.size());
  SHOW("Tag bitwidth is " << tagBitWidth)
  llvmType =
      llvm::StructType::create(ctx,
                               {llvm::Type::getIntNTy(ctx, tagBitWidth),
                                llvm::Type::getIntNTy(ctx, maxSize)},
                               parent->getFullNameWithChild(name), _isPacked);
  if (parent) {
    parent->unionTypes.push_back(this);
  }
}

void UnionType::findTagBitWidth(usize typeCount) {
  auto calc = 2;
  while (calc < typeCount) {
    calc <<= 1;
    tagBitWidth++;
  }
}

String UnionType::getName() const { return name; }

String UnionType::getFullName() const {
  return parent->getFullNameWithChild(name);
}

usize UnionType::getIndexOfName(const String &name) const {
  for (usize i = 0; i < subtypes.size(); i++) {
    if (subtypes.at(i).first == name) {
      return i;
    }
  }
  return subtypes.size();
}

Pair<bool, bool> UnionType::hasSubTypeWithName(const String &sname) const {
  for (const auto &sTy : subtypes) {
    if (sTy.first == sname) {
      return {true, sTy.second.has_value()};
    }
  }
  return {false, false};
}

QatType *UnionType::getSubTypeWithName(const String &sname) const {
  for (const auto &sTy : subtypes) {
    if (sTy.first == sname) {
      return sTy.second.value();
    }
  }
  return nullptr;
}

void UnionType::getMissingNames(Vec<String> &vals, Vec<String> &missing) const {
  for (const auto &sub : subtypes) {
    bool result = false;
    for (const auto &val : vals) {
      if (sub.first == val) {
        result = true;
        break;
      }
    }
    if (!result) {
      missing.push_back(sub.first);
    }
  }
}

usize UnionType::getSubTypeCount() const { return subtypes.size(); }

QatModule *UnionType::getParent() const { return parent; }

bool UnionType::isPacked() const { return isPack; }

usize UnionType::getTagBitwidth() const { return tagBitWidth; }

u64 UnionType::getDataBitwidth() const { return maxSize; }

bool UnionType::isAccessible(const utils::RequesterInfo &reqInfo) const {
  return visibility.isAccessible(reqInfo);
}

const utils::VisibilityInfo &UnionType::getVisibility() const {
  return visibility;
}

String UnionType::toString() const { return getFullName(); }

TypeKind UnionType::typeKind() const { return TypeKind::unionType; }

} // namespace qat::IR