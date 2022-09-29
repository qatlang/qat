#include "./say_sentence.hpp"
#include "../../IR/types/string_slice.hpp"

namespace qat::ast {

IR::Value *Say::emit(IR::Context *ctx) {
  SHOW("Say sentence emitting..")
  auto *mod = ctx->getMod();
  SHOW("Current block is: " << ctx->fn->getBlock()->getName())
  mod->linkNative(IR::NativeUnit::printf);
  auto              *printfFn = mod->getLLVMModule()->getFunction("printf");
  String             formatStr;
  Vec<IR::Value *>   valuesIR;
  Vec<llvm::Value *> llvmValues;
  for (auto *exp : expressions) {
    valuesIR.push_back(exp->emit(ctx));
    SHOW(
        "Say sentence member type: " << valuesIR.back()->getType()->toString());
  }
  std::function<void(IR::Value *, usize)> formatter = [&](IR::Value *value,
                                                          usize      index) {
    SHOW("Say sentence value type is: " << value->getType()->toString())
    auto *val = value;
    auto *typ = val->getType();
    if (typ->isReference()) {
      val->loadImplicitPointer(ctx->builder);
      val = new IR::Value(
          ctx->builder.CreateLoad(
              typ->asReference()->getSubType()->getLLVMType(), val->getLLVM()),
          typ->asReference()->getSubType(), false, IR::Nature::temporary);
      typ = typ->asReference()->getSubType();
      SHOW("Reference in say sentence of type " << typ->toString())
    }
    if (typ->isStringSlice()) {
      formatStr += "%.*s";
      if (llvm::isa<llvm::Constant>(val->getLLVM())) {
        auto *strStruct = (llvm::ConstantStruct *)val->getLLVM();
        llvmValues.push_back(strStruct->getAggregateElement(1u));
        llvmValues.push_back(strStruct->getAggregateElement(0u));
      } else if (llvm::isa<llvm::AllocaInst>(val->getLLVM())) {
        auto *strStruct = val->getLLVM();
        llvmValues.push_back(ctx->builder.CreateLoad(
            llvm::Type::getInt64Ty(ctx->llctx),
            ctx->builder.CreateStructGEP(
                IR::StringSliceType::get(ctx->llctx)->getLLVMType(), strStruct,
                1)));
        llvmValues.push_back(ctx->builder.CreateLoad(
            llvm::Type::getInt8Ty(ctx->llctx)->getPointerTo(),
            ctx->builder.CreateStructGEP(
                IR::StringSliceType::get(ctx->llctx)->getLLVMType(), strStruct,
                0)));
      } else {
        auto *strStruct = val->getLLVM();
        auto *tempAlloc = ctx->builder.CreateAlloca(
            IR::StringSliceType::get(ctx->llctx)->getLLVMType());
        ctx->builder.CreateStore(strStruct, tempAlloc);
        llvmValues.push_back(ctx->builder.CreateLoad(
            llvm::Type::getInt64Ty(ctx->llctx),
            ctx->builder.CreateStructGEP(
                IR::StringSliceType::get(ctx->llctx)->getLLVMType(), tempAlloc,
                1)));
        llvmValues.push_back(ctx->builder.CreateLoad(
            llvm::Type::getInt8Ty(ctx->llctx)->getPointerTo(),
            ctx->builder.CreateStructGEP(
                IR::StringSliceType::get(ctx->llctx)->getLLVMType(), tempAlloc,
                0)));
      }
    } else if (typ->isCString()) {
      formatStr += "%s";
      val->loadImplicitPointer(ctx->builder);
      llvmValues.push_back(val->getLLVM());
    } else if (typ->isInteger()) {
      formatStr += "%d";
      val->loadImplicitPointer(ctx->builder);
      llvmValues.push_back(val->getLLVM());
    } else if (typ->isUnsignedInteger()) {
      formatStr += "%u";
      val->loadImplicitPointer(ctx->builder);
      llvmValues.push_back(val->getLLVM());
    } else if (typ->isFloat()) {
      formatStr += "%f";
      val->loadImplicitPointer(ctx->builder);
      if (typ->asFloat()->getKind() != IR::FloatTypeKind::_64) {
        llvmValues.push_back(ctx->builder.CreateFPCast(
            val->getLLVM(), llvm::Type::getDoubleTy(ctx->llctx)));
      } else {
        llvmValues.push_back(val->getLLVM());
      }
    } else if (typ->isPointer()) {
      formatStr += "%p";
      val->loadImplicitPointer(ctx->builder);
      llvmValues.push_back(val->getLLVM());
    } else if (typ->isTuple()) {
      formatStr += "(";
      auto subTypes = typ->asTuple()->getSubTypes();
      for (usize i = 0; i < subTypes.size(); i++) {
        auto *subVal = val->getLLVM();
        subVal       = ctx->builder.CreateGEP(
            subTypes.at(i)->getLLVMType(), subVal,
            llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), i));
        subVal = ctx->builder.CreateLoad(subTypes.at(i)->getLLVMType(), subVal);
        formatter(new IR::Value(subVal, subTypes.at(i), val->isVariable(),
                                val->getNature()),
                  index);
        if (i != (subTypes.size() - 1)) {
          formatStr += "; ";
        }
      }
      formatStr += ")";
    } else if (typ->isArray()) {
      formatStr += "[";
      auto *subType = typ->asArray()->getElementType();
      for (usize i = 0; i < typ->asArray()->getLength(); i++) {
        auto *subVal = ctx->builder.CreateGEP(
            subType->getLLVMType(), val->getLLVM(),
            // TODO - Change index type to usize llvm equivalent
            llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), i));
        subVal = ctx->builder.CreateLoad(subType->getLLVMType(), subVal);
        formatter(
            new IR::Value(subVal, subType, val->isVariable(), val->getNature()),
            i);
        if (i != (typ->asArray()->getLength() - 1)) {
          formatStr += ", ";
        }
      }
      formatStr += "]";
    } else {
      ctx->Error("Cannot convert this value to a string",
                 expressions.at(index)->fileRange);
    }
  };
  for (usize i = 0; i < valuesIR.size(); i++) {
    formatter(valuesIR.at(i), i);
  }
  formatStr += "\n";
  Vec<llvm::Value *> values;
  values.push_back(ctx->builder.CreateGlobalStringPtr(
      formatStr, ctx->getGlobalStringName()));
  for (auto *val : llvmValues) {
    values.push_back(val);
  }
  llvmValues.clear();
  ctx->builder.CreateCall(printfFn->getFunctionType(), printfFn, values);
  return nullptr;
}

Json Say::toJson() const {
  Vec<JsonValue> exps;
  for (auto *exp : expressions) {
    exps.push_back(exp->toJson());
  }
  return Json()
      ._("nodeType", "saySentence")
      ._("values", exps)
      ._("fileRange", fileRange);
}

} // namespace qat::ast