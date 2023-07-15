#include "./float_literal.hpp"
#include "llvm/ADT/APFloat.h"
#include "llvm/IR/Instruction.h"

namespace qat::ast {

FloatLiteral::FloatLiteral(String _value, FileRange _fileRange)
    : ConstantExpression(std::move(_fileRange)), value(std::move(_value)) {}

IR::ConstantValue* FloatLiteral::emit(IR::Context* ctx) {
  if (isExpectedKind(ExpressionKind::assignable)) {
    ctx->Error("Float literals are not assignable", fileRange);
  }
  SHOW("Generating float literal")
  return new IR::ConstantValue(llvm::ConstantFP::get(llvm::Type::getDoubleTy(ctx->llctx), std::stof(value)),
                               IR::FloatType::get(IR::FloatTypeKind::_64, ctx->llctx));
}

String FloatLiteral::toString() const { return value; }

Json FloatLiteral::toJson() const {
  return Json()._("nodeType", "floatLiteral")._("value", value)._("fileRange", fileRange);
}

} // namespace qat::ast