#include "./integer.hpp"
#include "../../memory_tracker.hpp"
#include "../../utils/json.hpp"
#include "../value.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"

#define MAXIMUM_CONST_EXPR_BITWIDTH 64u

namespace qat::IR {

IntegerType::IntegerType(u64 _bitWidth, llvm::LLVMContext& llctx) : bitWidth(_bitWidth) {
  llvmType = llvm::IntegerType::get(llctx, bitWidth);
}

IntegerType* IntegerType::get(u64 bits, llvm::LLVMContext& llctx) {
  for (auto* typ : types) {
    if (typ->isInteger()) {
      if (typ->asInteger()->isBitWidth(bits)) {
        return typ->asInteger();
      }
    }
  }
  return new IntegerType(bits, llctx);
}

bool IntegerType::isBitWidth(u64 width) const { return bitWidth == width; }

TypeKind IntegerType::typeKind() const { return TypeKind::integer; }

u64 IntegerType::getBitwidth() const { return bitWidth; }

bool IntegerType::isTypeSized() const { return true; }

String IntegerType::toString() const { return "i" + std::to_string(bitWidth); }

bool IntegerType::canBePrerunGeneric() const { return bitWidth <= MAXIMUM_CONST_EXPR_BITWIDTH; }

Maybe<String> IntegerType::toPrerunGenericString(IR::PrerunValue* val) const {
  if (!canBePrerunGeneric()) {
    return None;
  }
  return std::to_string((i64)(*llvm::cast<llvm::ConstantInt>(val->getLLVMConstant())->getValue().getRawData()));
}

Maybe<bool> IntegerType::equalityOf(IR::PrerunValue* first, IR::PrerunValue* second) const {
  if (first->getType()->isSame(second->getType())) {
    return (*llvm::cast<llvm::ConstantInt>(first->getLLVMConstant())->getValue().getRawData()) ==
           (*llvm::cast<llvm::ConstantInt>(second->getLLVMConstant())->getValue().getRawData());
  } else {
    return false;
  }
}

} // namespace qat::IR