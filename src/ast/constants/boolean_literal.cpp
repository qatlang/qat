#include "./boolean_literal.hpp"

namespace qat::ast {

BooleanLiteral::BooleanLiteral(bool _value, FileRange _fileRange)
    : ConstantExpression(std::move(_fileRange)), value(_value) {}

IR::ConstantValue* BooleanLiteral::emit(IR::Context* ctx) {
  return new IR::ConstantValue(llvm::ConstantInt::getBool(ctx->llctx, value), IR::UnsignedType::getBool(ctx->llctx));
}

Json BooleanLiteral::toJson() const {
  return Json()._("nodeType", "booleanLiteral")._("value", value)._("fileRange", fileRange);
}

} // namespace qat::ast