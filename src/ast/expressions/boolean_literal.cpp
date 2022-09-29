#include "./boolean_literal.hpp"

namespace qat::ast {

BooleanLiteral::BooleanLiteral(bool _value, utils::FileRange _fileRange)
    : Expression(std::move(_fileRange)), value(_value) {}

IR::Value *BooleanLiteral::emit(IR::Context *ctx) {
  return new IR::Value(llvm::ConstantInt::getBool(ctx->llctx, value),
                       IR::UnsignedType::get(1u, ctx->llctx), false,
                       IR::Nature::pure);
}

Json BooleanLiteral::toJson() const {
  return Json()
      ._("nodeType", "booleanLiteral")
      ._("value", value)
      ._("fileRange", fileRange);
}

} // namespace qat::ast