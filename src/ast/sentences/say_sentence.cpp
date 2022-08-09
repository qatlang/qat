#include "./say_sentence.hpp"

namespace qat::ast {

IR::Value *Say::emit(IR::Context *ctx) {
  auto *mod = ctx->getMod();
  mod->linkNative(IR::NativeUnit::printf);
  auto            *printfFn = mod->getLLVMModule()->getFunction("printf");
  String           formatStr;
  Vec<IR::Value *> valuesIR;
  for (auto *exp : expressions) {
    valuesIR.push_back(exp->emit(ctx));
  }
  Vec<llvm::Value *>                      llvmValues;
  std::function<void(IR::Value *, usize)> formatter = [&](IR::Value *val,
                                                          usize      index) {
    auto *typ = val->getType();
    if (typ->isStringSlice()) {
      formatStr += "%s";
      llvmValues.push_back(val->getLLVM());
    } else if (typ->isInteger()) {
      formatStr += "%d";
      llvmValues.push_back(val->getLLVM());
    } else if (typ->isUnsignedInteger()) {
      formatStr += "%u";
      llvmValues.push_back(val->getLLVM());
    } else if (typ->isFloat()) {
      formatStr += "%f";
      llvmValues.push_back(val->getLLVM());
    } else if (typ->isPointer()) {
      formatStr += "%p";
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
        formatStr += ", ";
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
  values.push_back(ctx->builder.CreateGlobalStringPtr(formatStr, "str"));
  for (auto *val : llvmValues) {
    values.push_back(val);
  }
  llvmValues.clear();
  ctx->builder.CreateCall(printfFn->getFunctionType(), printfFn, values);
  return nullptr;
}

nuo::Json Say::toJson() const {
  Vec<nuo::JsonValue> exps;
  for (auto *exp : expressions) {
    exps.push_back(exp->toJson());
  }
  return nuo::Json()
      ._("nodeType", "saySentence")
      ._("values", exps)
      ._("fileRange", fileRange);
}

} // namespace qat::ast