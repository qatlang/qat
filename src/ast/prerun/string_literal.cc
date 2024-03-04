#include "./string_literal.hpp"
#include "../../IR/types/string_slice.hpp"
#include "llvm/IR/Constants.h"

namespace qat::ast {

String StringLiteral::get_value() const { return value; }

void StringLiteral::addValue(const String& val, const FileRange& fRange) {
  value += val;
  fileRange = FileRange(fileRange, fRange);
}

ir::PrerunValue* StringLiteral::emit(EmitCtx* ctx) {
  return ir::PrerunValue::get(
      llvm::ConstantStruct::get(
          llvm::cast<llvm::StructType>(ir::StringSliceType::get(ctx->irCtx)->get_llvm_type()),
          // NOTE - This usage of llvm::IRBuilder is allowed as it creates a constant without requiring a function
          {ctx->irCtx->builder.CreateGlobalStringPtr(value, ctx->irCtx->get_global_string_name(), 0U,
                                                     ctx->mod->get_llvm_module()),
           llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->irCtx->llctx), value.length())}),
      ir::StringSliceType::get(ctx->irCtx));
}

String StringLiteral::to_string() const { return '"' + value + '"'; }

Json StringLiteral::to_json() const {
  return Json()._("nodeType", "stringLiteral")._("value", value)._("fileRange", fileRange);
}

} // namespace qat::ast