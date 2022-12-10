#include "./string_literal.hpp"
#include "../../IR/types/string_slice.hpp"
#include "llvm/IR/Constants.h"

namespace qat::ast {

StringLiteral::StringLiteral(String _value, FileRange _fileRange)
    : ConstantExpression(std::move(_fileRange)), value(std::move(_value)) {}

String StringLiteral::get_value() const { return value; }

void StringLiteral::addValue(const String& val, const FileRange& fRange) {
  value += val;
  fileRange = FileRange(fileRange, fRange);
}

IR::ConstantValue* StringLiteral::emit(IR::Context* ctx) {
  return new IR::ConstantValue(
      llvm::ConstantStruct::get(
          (llvm::StructType*)IR::StringSliceType::get(ctx->llctx)->getLLVMType(),
          {ctx->builder.CreateGlobalStringPtr(value, ctx->getGlobalStringName(), 0U, ctx->getMod()->getLLVMModule()),
           llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), value.length())}),
      IR::StringSliceType::get(ctx->llctx));
}

Json StringLiteral::toJson() const {
  return Json()._("nodeType", "stringLiteral")._("value", value)._("fileRange", fileRange);
}

} // namespace qat::ast