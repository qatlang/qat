#include "./string_literal.hpp"
#include "../../IR/types/string_slice.hpp"

namespace qat::ast {

StringLiteral::StringLiteral(String _value, utils::FileRange _fileRange)
    : Expression(std::move(_fileRange)), value(std::move(_value)) {}

String StringLiteral::get_value() const { return value; }

IR::Value *StringLiteral::emit(IR::Context *ctx) {
  return new IR::Value(ctx->builder.CreateGlobalStringPtr(
                           value, "str", 0U, ctx->getMod()->getLLVMModule()),
                       IR::StringSliceType::get(ctx->llctx), false,
                       IR::Nature::pure);
}

nuo::Json StringLiteral::toJson() const {
  return nuo::Json()
      ._("nodeType", "stringLiteral")
      ._("value", value)
      ._("fileRange", fileRange);
}

} // namespace qat::ast