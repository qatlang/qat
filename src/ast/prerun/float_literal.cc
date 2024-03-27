#include "./float_literal.hpp"
#include "llvm/ADT/APFloat.h"

namespace qat::ast {

ir::PrerunValue* FloatLiteral::emit(EmitCtx* ctx) {
  SHOW("Generating float literal for " << value)
  ir::Type* floatResTy = nullptr;
  if (is_type_inferred()) {
    if (inferredType->is_float() || (inferredType->is_ctype() && inferredType->as_ctype()->get_subtype()->is_float())) {
      floatResTy = inferredType;
    } else {
      ctx->Error("The type inferred from scope is " + ctx->color(inferredType->to_string()) +
                     " but this expression is expected to have a floating point type",
                 fileRange);
    }
  } else {
    floatResTy = ir::FloatType::get(ir::FloatTypeKind::_64, ctx->irCtx->llctx);
  }
  return ir::PrerunValue::get(llvm::ConstantFP::get(floatResTy->get_llvm_type(), value), floatResTy);
}

String FloatLiteral::to_string() const { return value; }

Json FloatLiteral::to_json() const {
  return Json()._("nodeType", "floatLiteral")._("value", value)._("fileRange", fileRange);
}

} // namespace qat::ast