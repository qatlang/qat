#include "./integer_literal.hpp"

namespace qat::ast {

ir::PrerunValue* IntegerLiteral::emit(EmitCtx* ctx) {
  if (isTypeInferred() &&
      (!inferredType->is_integer() && !inferredType->is_unsigned_integer() &&
       (!bits.has_value() && !inferredType->is_float()) &&
       (!bits.has_value() && inferredType->is_ctype() && !inferredType->as_ctype()->get_subtype()->is_float()) &&
       (inferredType->is_ctype() && !(inferredType->as_ctype()->get_subtype()->is_unsigned_integer() ||
                                      inferredType->as_ctype()->get_subtype()->is_integer())))) {
    if (bits.has_value()) {
      ctx->Error("The inferred type is " + ctx->color(inferredType->to_string()) +
                     ". The only supported types for this literal are signed & unsigned integers",
                 fileRange);
    } else {
      ctx->Error(
          "This inferred type is " + ctx->color(inferredType->to_string()) +
              ". The only supported types for this literal are signed integers, unsigned integers and floating point types",
          fileRange);
    }
  }
  ir::Type* resTy = isTypeInferred()
                        ? (inferredType->is_ctype() ? inferredType->as_ctype()->get_subtype() : inferredType)
                        : ir::IntegerType::get(32, ctx->irCtx);
  if (bits.has_value() && !ctx->mod->has_integer_bitwidth(bits.value().first)) {
    ctx->Error("The custom integer bitwidth " + ctx->color(std::to_string(bits.value().first)) +
                   " is not brought into the current module",
               bits.value().second);
  }
  String numValue = value;
  if (value.find('_') != String::npos) {
    numValue = "";
    for (auto intCh : value) {
      if (intCh != '_') {
        numValue += intCh;
      }
    }
  }
  // NOLINTBEGIN(readability-magic-numbers)
  if (resTy->is_float()) {
    return ir::PrerunValue::get(llvm::ConstantFP::get(inferredType->get_llvm_type(), numValue), inferredType);
  } else {
    return ir::PrerunValue::get(
        llvm::ConstantInt::get(
            isTypeInferred() ? llvm::cast<llvm::IntegerType>(inferredType->get_llvm_type())
                             : llvm::Type::getIntNTy(ctx->irCtx->llctx, bits.has_value() ? bits.value().first : 32u),
            numValue, 10u),
        isTypeInferred() ? inferredType
                         : ir::IntegerType::get(bits.has_value() ? bits.value().first : 32u, ctx->irCtx));
  }
  // NOLINTEND(readability-magic-numbers)
}

String IntegerLiteral::to_string() const { return value; }

Json IntegerLiteral::to_json() const {
  return Json()._("nodeType", "integerLiteral")._("value", value)._("fileRange", fileRange);
}

} // namespace qat::ast