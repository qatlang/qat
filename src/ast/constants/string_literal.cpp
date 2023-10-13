#include "./string_literal.hpp"
#include "../../IR/types/string_slice.hpp"
#include "llvm/IR/Constants.h"

namespace qat::ast {

StringLiteral::StringLiteral(String _value, FileRange _fileRange)
    : PrerunExpression(std::move(_fileRange)), value(std::move(_value)) {}

String StringLiteral::get_value() const { return value; }

void StringLiteral::addValue(const String& val, const FileRange& fRange) {
  value += val;
  fileRange = FileRange(fileRange, fRange);
}

IR::PrerunValue* StringLiteral::emit(IR::Context* ctx) {
  return new IR::PrerunValue(
      llvm::ConstantStruct::get(
          llvm::cast<llvm::StructType>(IR::StringSliceType::get(ctx->llctx)->getLLVMType()),
          // NOTE - This usage of llvm::IRBuilder is allowed as it creates a constant without requiring a function
          {ctx->builder.CreateGlobalStringPtr(value, ctx->getGlobalStringName(), 0U, ctx->getMod()->getLLVMModule()),
           llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), value.length())}),
      IR::StringSliceType::get(ctx->llctx));
}

String StringLiteral::toString() const { return '"' + value + '"'; }

Json StringLiteral::toJson() const {
  return Json()._("nodeType", "stringLiteral")._("value", value)._("fileRange", fileRange);
}

} // namespace qat::ast