#include "./custom_integer_literal.hpp"
#include "../../utils/json.hpp"
#include <string>

namespace qat::ast {

CustomIntegerLiteral::CustomIntegerLiteral(String _value, bool _isUnsigned, u32 _bitWidth, FileRange _fileRange)
    : ConstantExpression(std::move(_fileRange)), value(std::move(_value)), bitWidth(_bitWidth),
      isUnsigned(_isUnsigned) {}

IR::ConstantValue* CustomIntegerLiteral::emit(IR::Context* ctx) {
  if (isExpectedKind(ExpressionKind::assignable)) {
    ctx->Error("Integer literals are not assignable", fileRange);
  }
  // NOLINTBEGIN(readability-magic-numbers)
  return new IR::ConstantValue(llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, bitWidth), value, 10u),
                               (isUnsigned ? (IR::QatType*)IR::UnsignedType::get(bitWidth, ctx->llctx)
                                           : (IR::QatType*)IR::IntegerType::get(bitWidth, ctx->llctx)));
  // NOLINTEND(readability-magic-numbers)
}

Json CustomIntegerLiteral::toJson() const {
  return Json()
      ._("nodeType", "customIntegerLiteral")
      ._("isUnsigned", isUnsigned)
      ._("bitWidth", (unsigned long long)bitWidth)
      ._("value", value)
      ._("fileRange", fileRange);
}

String CustomIntegerLiteral::toString() const { return value + "_" + std::to_string(bitWidth); }

} // namespace qat::ast