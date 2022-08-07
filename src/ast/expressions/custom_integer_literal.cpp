#include "./custom_integer_literal.hpp"
#include <nuo/json.hpp>

namespace qat::ast {

CustomIntegerLiteral::CustomIntegerLiteral(String _value, bool _isUnsigned,
                                           u32              _bitWidth,
                                           utils::FileRange _fileRange)
    : Expression(std::move(_fileRange)), value(std::move(_value)),
      bitWidth(_bitWidth), isUnsigned(_isUnsigned) {}

IR::Value *CustomIntegerLiteral::emit(IR::Context *ctx) {
  if (isExpectedKind(ExpressionKind::assignable)) {
    ctx->Error("Integer literals are not assignable", fileRange);
  }
  // NOLINTBEGIN(readability-magic-numbers)
  return new IR::Value(
      llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, bitWidth), value,
                             10u),
      (isUnsigned ? (IR::QatType *)IR::UnsignedType::get(bitWidth, ctx->llctx)
                  : (IR::QatType *)IR::IntegerType::get(bitWidth, ctx->llctx)),
      false, IR::Nature::pure);
  // NOLINTEND(readability-magic-numbers)
}

nuo::Json CustomIntegerLiteral::toJson() const {
  return nuo::Json()
      ._("nodeType", "customIntegerLiteral")
      ._("isUnsigned", isUnsigned)
      ._("bitWidth", (unsigned long long)bitWidth)
      ._("value", value)
      ._("fileRange", fileRange);
}

} // namespace qat::ast