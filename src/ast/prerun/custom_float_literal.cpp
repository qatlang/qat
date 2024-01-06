#include "./custom_float_literal.hpp"
#include "llvm/ADT/APFloat.h"

namespace qat::ast {

CustomFloatLiteral::CustomFloatLiteral(String _value, String _kind, FileRange _fileRange)
    : PrerunExpression(std::move(_fileRange)), value(std::move(_value)), kind(std::move(_kind)) {}

IR::PrerunValue* CustomFloatLiteral::emit(IR::Context* ctx) {
  IR::QatType* floatResTy = nullptr;
  if (isTypeInferred()) {
    if (inferredType->isFloat() || (inferredType->isCType() && inferredType->asCType()->getSubType()->isFloat())) {
      if (!kind.empty() && inferredType->toString() != kind) {
        ctx->Error("The suffix provided here is " + ctx->highlightError(kind) + " but the inferred type is " +
                       ctx->highlightError(inferredType->toString()),
                   fileRange);
      }
      floatResTy = inferredType;
    } else {
      ctx->Error("The type inferred from scope is " + ctx->highlightError(inferredType->toString()) +
                     " but this literal is expected to have a floating point type",
                 fileRange);
    }
  } else if (!kind.empty()) {
    if (kind == "f16") {
      floatResTy = IR::FloatType::get(IR::FloatTypeKind::_16, ctx->llctx);
    } else if (kind == "fbrain") {
      floatResTy = IR::FloatType::get(IR::FloatTypeKind::_brain, ctx->llctx);
    } else if (kind == "f32") {
      floatResTy = IR::FloatType::get(IR::FloatTypeKind::_32, ctx->llctx);
    } else if (kind == "f64") {
      floatResTy = IR::FloatType::get(IR::FloatTypeKind::_64, ctx->llctx);
    } else if (kind == "f80") {
      floatResTy = IR::FloatType::get(IR::FloatTypeKind::_80, ctx->llctx);
    } else if (kind == "f128") {
      floatResTy = IR::FloatType::get(IR::FloatTypeKind::_128, ctx->llctx);
    } else if (kind == "f128ppc") {
      floatResTy = IR::FloatType::get(IR::FloatTypeKind::_128PPC, ctx->llctx);
    } else if (IR::cTypeKindFromString(kind).has_value()) {
      floatResTy = IR::CType::getFromCTypeKind(IR::cTypeKindFromString(kind).value(), ctx);
    } else {
      ctx->Error("Invalid suffix provided for custom float literal", fileRange);
    }
  } else {
    floatResTy = IR::FloatType::get(IR::FloatTypeKind::_64, ctx->llctx);
  }
  return new IR::PrerunValue(llvm::ConstantFP::get(floatResTy->getLLVMType(), value), floatResTy);
}

String CustomFloatLiteral::toString() const { return value + "_" + kind; }

Json CustomFloatLiteral::toJson() const {
  return Json()._("nodeType", "customFloatLiteral")._("nature", kind)._("value", value)._("fileRange", fileRange);
}

} // namespace qat::ast