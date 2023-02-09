#include "./string_slice.hpp"
#include "../../IR/logic.hpp"
#include "../../memory_tracker.hpp"
#include "../../show.hpp"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

StringSliceType::StringSliceType(llvm::LLVMContext& ctx) {
  if (llvm::StructType::getTypeByName(ctx, "str")) {
    llvmType = llvm::StructType::getTypeByName(ctx, "str");
  } else {
    llvmType = llvm::StructType::create(
        {llvm::PointerType::get(llvm::Type::getInt8Ty(ctx), 0U), llvm::Type::getInt64Ty(ctx)}, "str");
  }
}

StringSliceType* StringSliceType::get(llvm::LLVMContext& ctx) {
  for (auto* typ : types) {
    if (typ->typeKind() == TypeKind::stringSlice) {
      return (StringSliceType*)typ;
    }
  }
  return new StringSliceType(ctx);
}

bool StringSliceType::canBeConstGeneric() const { return true; }

Maybe<String> StringSliceType::toConstGenericString(IR::ConstantValue* val) const {
  auto* initial = llvm::cast<llvm::ConstantDataArray>(val->getLLVMConstant()->getAggregateElement(0u)->getOperand(0u));
  if (initial->getNumElements() == 1u) {
    return "\"\"";
  } else {
    String value;
    for (usize i = 0; i < initial->getRawDataValues().size(); i++) {
      if (initial->getRawDataValues().str().at(i) == '\0') {
        SHOW("Null character found at: " << i << " with total size: " << initial->getRawDataValues().size())
      } else {
        value += initial->getRawDataValues().str().at(i);
      }
    }
    return '"' + value + '"';
  }
}

Maybe<bool> StringSliceType::equalityOf(IR::ConstantValue* first, IR::ConstantValue* second) const {
  return IR::Logic::compareConstantStrings(
      first->getLLVMConstant()->getAggregateElement(0u), first->getLLVMConstant()->getAggregateElement(1u),
      second->getLLVMConstant()->getAggregateElement(0u), second->getLLVMConstant()->getAggregateElement(1u),
      first->getLLVMConstant()->getContext());
}

TypeKind StringSliceType::typeKind() const { return TypeKind::stringSlice; }

String StringSliceType::toString() const { return "str"; }

Json StringSliceType::toJson() const { return Json()._("type", "str"); }

} // namespace qat::IR