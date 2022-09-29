#include "./unsigned_literal.hpp"

namespace qat::ast {

UnsignedLiteral::UnsignedLiteral(String _value, utils::FileRange _fileRange)
    : value(_value), Expression(_fileRange) {}

IR::Value *UnsignedLiteral::emit(IR::Context *ctx) {
  if (getExpectedKind() == ExpressionKind::assignable) {
    ctx->Error("Unsigned literals are not assignable", fileRange);
  }
  // NOLINTBEGIN(readability-magic-numbers)
  return new IR::Value(
      llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx->llctx), value, 10u),
      IR::UnsignedType::get(32, ctx->llctx), false, IR::Nature::pure);
  // NOLINTEND(readability-magic-numbers)
}

Json UnsignedLiteral::toJson() const {
  return Json()
      ._("nodeType", "unsignedLiteral")
      ._("value", value)
      ._("fileRange", fileRange);
}

} // namespace qat::ast