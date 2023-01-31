#include "./unsigned.hpp"
#include "../../memory_tracker.hpp"
#include "../value.hpp"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

UnsignedType::UnsignedType(u64 _bitWidth, llvm::LLVMContext& ctx, bool _isBool) : bitWidth(_bitWidth), isBool(_isBool) {
  llvmType = llvm::IntegerType::get(ctx, bitWidth);
}

UnsignedType* UnsignedType::get(u64 bits, llvm::LLVMContext& ctx) {
  for (auto* typ : types) {
    if (typ->isUnsignedInteger()) {
      if (typ->asUnsignedInteger()->isBitWidth(bits) && !typ->asUnsignedInteger()->isBool) {
        return typ->asUnsignedInteger();
      }
    }
  }
  return new UnsignedType(bits, ctx, false);
}

UnsignedType* UnsignedType::getBool(llvm::LLVMContext& ctx) {
  for (auto* typ : types) {
    if (typ->isUnsignedInteger()) {
      if (typ->asUnsignedInteger()->isBool) {
        return typ->asUnsignedInteger();
      }
    }
  }
  return new UnsignedType(1u, ctx, true);
}

u64 UnsignedType::getBitwidth() const { return bitWidth; }

bool UnsignedType::isBitWidth(u64 width) const { return bitWidth == width; }

bool UnsignedType::isBoolean() const { return isBool; }

TypeKind UnsignedType::typeKind() const { return TypeKind::unsignedInteger; }

String UnsignedType::toString() const { return isBool ? "bool" : ("u" + std::to_string(bitWidth)); }

Json UnsignedType::toJson() const { return Json()._("type", "unsigned")._("bitWidth", std::to_string(bitWidth)); }

bool UnsignedType::canBeConstGeneric() const { return bitWidth <= 64u; }

Maybe<String> UnsignedType::toConstGenericString(IR::ConstantValue* val) const {
  if (!canBeConstGeneric()) {
    return None;
  }
  return std::to_string((*llvm::cast<llvm::ConstantInt>(val->getLLVMConstant())->getValue().getRawData()));
}

Maybe<bool> UnsignedType::equalityOf(IR::ConstantValue* first, IR::ConstantValue* second) const {
  if (!canBeConstGeneric()) {
    return None;
  }
  if (first->getType()->isSame(second->getType())) {
    return (*llvm::cast<llvm::ConstantInt>(first->getLLVMConstant())->getValue().getRawData()) ==
           (*llvm::cast<llvm::ConstantInt>(second->getLLVMConstant())->getValue().getRawData());
  } else {
    return false;
  }
}

} // namespace qat::IR