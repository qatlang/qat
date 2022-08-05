#include "./sum.hpp"
#include "../qat_module.hpp"
#include "qat_type.hpp"
#include "type_kind.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/TypeSize.h"

#define TwoPow8           256
#define TwoPow16          65536
#define SecondTagBitwidth 16
#define ThirdTagBitwidth  32

namespace qat::IR {

SumType::SumType(String _name, Vec<QatType *> _subTypes, llvm::LLVMContext &ctx)
    : name(std::move(_name)), subTypes(std::move(_subTypes)) {
  Vec<llvm::Type *> llvmSubTypes;
  u64               index           = 0;
  u64               lastHighestSize = 0;
  for (usize i = 0; i < subTypes.size(); i++) {
    llvmSubTypes.push_back(subTypes.at(i)->getLLVMType());
    auto *sizeCInt = llvm::ConstantInt::get(
        llvm::Type::getInt64Ty(ctx),
        parent->getLLVMModule()->getDataLayout().getTypeStoreSizeInBits(
            llvmSubTypes.back()));
    if (lastHighestSize < sizeCInt->getValue().getZExtValue()) {
      index           = i;
      lastHighestSize = sizeCInt->getValue().getZExtValue();
    }
  }
  if (subTypes.size() > TwoPow8) {
    if (subTypes.size() <= TwoPow16) {
      tagBitwidth = SecondTagBitwidth;
    } else {
      tagBitwidth = ThirdTagBitwidth;
    }
  }
  llvmType = llvm::StructType::create(
      {llvm::Type::getIntNTy(ctx, tagBitwidth), llvmSubTypes.at(index)}, name,
      false);
}

String SumType::getName() const { return name; }

String SumType::getFullName() const {
  return parent->getFullName() + ":" + name;
}

u32 SumType::getSubtypeCount() const { return subTypes.size(); }

QatType *SumType::getSubtypeAt(usize index) { return subTypes.at(index); }

bool SumType::isCompatible(QatType *other) const {
  if (isSame(other)) {
    return true;
  }
  for (auto *typ : subTypes) {
    if (typ->isSame(other)) {
      return true;
    } else if (typ->typeKind() == TypeKind::sumType) {
      if (((SumType *)typ)->isCompatible(other)) {
        return true;
      }
    }
  }
  return false;
}

String SumType::toString() const { return name; }

nuo::Json SumType::toJson() const {
  // TODO
}

} // namespace qat::IR