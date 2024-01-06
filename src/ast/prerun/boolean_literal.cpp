#include "./boolean_literal.hpp"

namespace qat::ast {

BooleanLiteral::BooleanLiteral(bool _value, FileRange _fileRange)
    : PrerunExpression(std::move(_fileRange)), value(_value) {}

IR::PrerunValue* BooleanLiteral::emit(IR::Context* ctx) {
  return new IR::PrerunValue(llvm::ConstantInt::getBool(ctx->llctx, value), IR::UnsignedType::getBool(ctx));
}

String BooleanLiteral::toString() const { return value ? "true" : "false"; }

Json BooleanLiteral::toJson() const {
  return Json()._("nodeType", "booleanLiteral")._("value", value)._("fileRange", fileRange);
}

} // namespace qat::ast