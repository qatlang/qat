#include "./custom_float_literal.hpp"
#include "llvm/ADT/APFloat.h"

namespace qat::ast {

CustomFloatLiteral::CustomFloatLiteral(String _value, String _kind, FileRange _fileRange)
    : ConstantExpression(std::move(_fileRange)), value(std::move(_value)), kind(std::move(_kind)) {}

IR::ConstantValue* CustomFloatLiteral::emit(IR::Context* ctx) {
  if (isExpectedKind(ExpressionKind::assignable)) {
    ctx->Error("Float literals are not assignable", fileRange);
  }
  if (kind == "fhalf") {
    return new IR::ConstantValue(llvm::ConstantFP::get(llvm::Type::getHalfTy(ctx->llctx), std::stof(value)),
                                 IR::FloatType::get(IR::FloatTypeKind::_half, ctx->llctx));
  } else if (kind == "fbrain") {
    return new IR::ConstantValue(llvm::ConstantFP::get(llvm::Type::getBFloatTy(ctx->llctx), std::stof(value)),
                                 IR::FloatType::get(IR::FloatTypeKind::_brain, ctx->llctx));
  } else if (kind == "f32") {
    return new IR::ConstantValue(llvm::ConstantFP::get(llvm::Type::getFloatTy(ctx->llctx), std::stof(value)),
                                 IR::FloatType::get(IR::FloatTypeKind::_32, ctx->llctx));
  } else if (kind == "f64") {
    return new IR::ConstantValue(llvm::ConstantFP::get(llvm::Type::getDoubleTy(ctx->llctx), std::stod(value)),
                                 IR::FloatType::get(IR::FloatTypeKind::_64, ctx->llctx));
  } else if (kind == "f80") {
    return new IR::ConstantValue(llvm::ConstantFP::get(llvm::Type::getX86_FP80Ty(ctx->llctx), std::stod(value)),
                                 IR::FloatType::get(IR::FloatTypeKind::_80, ctx->llctx));
  } else if (kind == "f128") {
    return new IR::ConstantValue(llvm::ConstantFP::get(llvm::Type::getFP128Ty(ctx->llctx), std::stod(value)),
                                 IR::FloatType::get(IR::FloatTypeKind::_128, ctx->llctx));
  } else if (kind == "f128ppc") {
    return new IR::ConstantValue(llvm::ConstantFP::get(llvm::Type::getPPC_FP128Ty(ctx->llctx), std::stod(value)),
                                 IR::FloatType::get(IR::FloatTypeKind::_128PPC, ctx->llctx));
  } else {
    ctx->Error("Invalid specification for custom float literal", fileRange);
  }
}

Json CustomFloatLiteral::toJson() const {
  return Json()._("nodeType", "customFloatLiteral")._("nature", kind)._("value", value)._("fileRange", fileRange);
}

} // namespace qat::ast