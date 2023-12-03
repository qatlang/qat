#include "./string_slice.hpp"
#include "../../IR/logic.hpp"
#include "../../memory_tracker.hpp"
#include "../../show.hpp"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

StringSliceType::StringSliceType(llvm::LLVMContext& llctx, bool _isPacked) : isPack(_isPacked) {
  linkingName = "qat'str" + String(isPack ? ":[pack]" : "");
  if (llvm::StructType::getTypeByName(llctx, linkingName)) {
    llvmType = llvm::StructType::getTypeByName(llctx, linkingName);
  } else {
    llvmType = llvm::StructType::create(
        {
            llvm::PointerType::get(llvm::Type::getInt8Ty(llctx), 0U),
            llvm::Type::getInt64Ty(llctx),
        },
        linkingName, isPack);
  }
}

bool StringSliceType::isPacked() const { return isPack; }

bool StringSliceType::isTypeSized() const { return true; }

StringSliceType* StringSliceType::get(llvm::LLVMContext& llctx, bool isPacked) {
  for (auto* typ : types) {
    if (typ->typeKind() == TypeKind::stringSlice && (((StringSliceType*)typ)->isPack = isPacked)) {
      return (StringSliceType*)typ;
    }
  }
  return new StringSliceType(llctx, isPacked);
}

bool StringSliceType::canBePrerunGeneric() const { return true; }

Maybe<String> StringSliceType::toPrerunGenericString(IR::PrerunValue* val) const {
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

Maybe<bool> StringSliceType::equalityOf(IR::PrerunValue* first, IR::PrerunValue* second) const {
  return IR::Logic::compareConstantStrings(
      first->getLLVMConstant()->getAggregateElement(0u), first->getLLVMConstant()->getAggregateElement(1u),
      second->getLLVMConstant()->getAggregateElement(0u), second->getLLVMConstant()->getAggregateElement(1u),
      first->getLLVMConstant()->getContext());
}

TypeKind StringSliceType::typeKind() const { return TypeKind::stringSlice; }

String StringSliceType::toString() const { return "str" + String(isPack ? ":[pack]" : ""); }

} // namespace qat::IR