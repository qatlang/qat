#include "./string_slice.hpp"
#include "../../IR/logic.hpp"
#include "../../show.hpp"
#include "../context.hpp"

#include <errno.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>

namespace qat::ir {

StringSliceType::StringSliceType(ir::Ctx* irCtx, bool _isPacked) : isPack(_isPacked) {
  linkingName = "qat'str" + String(isPack ? ":[pack]" : "");
  if (llvm::StructType::getTypeByName(irCtx->llctx, linkingName)) {
    llvmType = llvm::StructType::getTypeByName(irCtx->llctx, linkingName);
  } else {
    llvmType = llvm::StructType::create(
        {
            llvm::PointerType::get(llvm::Type::getInt8Ty(irCtx->llctx), 0U),
            llvm::Type::getIntNTy(irCtx->llctx,
                                  irCtx->clangTargetInfo->getTypeWidth(irCtx->clangTargetInfo->getSizeType())),
        },
        linkingName, isPack);
  }
}

ir::PrerunValue* StringSliceType::create_value(ir::Ctx* irCtx, String value) {
  auto strTy = ir::StringSliceType::get(irCtx);
  return ir::PrerunValue::get(
      llvm::ConstantStruct::get(
          llvm::cast<llvm::StructType>(strTy->get_llvm_type()),
          {irCtx->builder.CreateGlobalStringPtr(value, irCtx->get_global_string_name(),
                                                irCtx->dataLayout.value().getDefaultGlobalsAddressSpace()),
           llvm::ConstantInt::get(llvm::Type::getInt64Ty(irCtx->llctx), value.length())}),
      strTy);
}

bool StringSliceType::isPacked() const { return isPack; }

bool StringSliceType::is_type_sized() const { return true; }

StringSliceType* StringSliceType::get(ir::Ctx* irCtx, bool isPacked) {
  for (auto* typ : allTypes) {
    if (typ->type_kind() == TypeKind::stringSlice && (((StringSliceType*)typ)->isPack = isPacked)) {
      return (StringSliceType*)typ;
    }
  }
  return new StringSliceType(irCtx, isPacked);
}

String StringSliceType::value_to_string(ir::PrerunValue* value) {
  auto* initial =
      llvm::cast<llvm::ConstantDataArray>(value->get_llvm_constant()->getAggregateElement(0u)->getOperand(0u));
  if (initial->getNumElements() == 1u) {
    return "";
  } else {
    auto tempStr = String(initial->getRawDataValues().data(), initial->getRawDataValues().size());
    if (tempStr[tempStr.size() - 1] == '\0') {
      return tempStr.substr(0, tempStr.size() - 1);
    }
    return tempStr;
  }
}

Maybe<String> StringSliceType::to_prerun_generic_string(ir::PrerunValue* val) const {
  auto* initial =
      llvm::cast<llvm::ConstantDataArray>(val->get_llvm_constant()->getAggregateElement(0u)->getOperand(0u));
  if (initial->getNumElements() == 1u) {
    return "\"\"";
  } else {
    auto tempStr = initial->getRawDataValues().str();
    if (tempStr[tempStr.size() - 1] == '\0') {
      return '"' + tempStr.substr(0, tempStr.size() - 1) + '"';
    }
    return '"' + tempStr + '"';
  }
}

Maybe<bool> StringSliceType::equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const {
  return ir::Logic::compareConstantStrings(first->get_llvm_constant()->getAggregateElement(0u),
                                           first->get_llvm_constant()->getAggregateElement(1u),
                                           second->get_llvm_constant()->getAggregateElement(0u),
                                           second->get_llvm_constant()->getAggregateElement(1u), irCtx->llctx);
}

TypeKind StringSliceType::type_kind() const { return TypeKind::stringSlice; }

String StringSliceType::to_string() const { return "str" + String(isPack ? ":[pack]" : ""); }

} // namespace qat::ir
