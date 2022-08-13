#include "./float_literal.hpp"
#include "llvm/ADT/APFloat.h"
#include "llvm/IR/Instruction.h"

namespace qat::ast {

FloatLiteral::FloatLiteral(String _value, utils::FileRange _fileRange)
    : Expression(std::move(_fileRange)), value(std::move(_value)) {}

IR::Value *FloatLiteral::emit(IR::Context *ctx) {
  if (isExpectedKind(ExpressionKind::assignable)) {
    ctx->Error("Float literals are not assignable", fileRange);
  }
  SHOW("Generating float literal")
  return new IR::Value(
      (llvm::ConstantFP *)llvm::ConstantFP::get(
          llvm::Type::getFloatTy(ctx->llctx), std::stof(value)),
      IR::FloatType::get(IR::FloatTypeKind::_32, ctx->llctx), false,
      IR::Nature::pure);
}

nuo::Json FloatLiteral::toJson() const {
  return nuo::Json()
      ._("nodeType", "floatLiteral")
      ._("value", value)
      ._("fileRange", fileRange);
}

} // namespace qat::ast