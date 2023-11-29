#include "./unsigned_literal.hpp"

namespace qat::ast {

UnsignedLiteral::UnsignedLiteral(String _value, FileRange _fileRange)
    : PrerunExpression(std::move(_fileRange)), value(std::move(_value)) {}

IR::PrerunValue* UnsignedLiteral::emit(IR::Context* ctx) {
  if (isTypeInferred() && !inferredType->isUnsignedInteger()) {
    ctx->Error("The inferred type of this expression is " + inferredType->toString() +
                   ". Unsigned integer literal expects an unsigned integer type to be provided for type inference",
               fileRange);
  }
  // NOLINTBEGIN(readability-magic-numbers)
  return new IR::PrerunValue(llvm::ConstantInt::get(inferredType
                                                        ? llvm::dyn_cast<llvm::IntegerType>(inferredType->getLLVMType())
                                                        : llvm::Type::getInt32Ty(ctx->llctx),
                                                    value, 10u),
                             isTypeInferred() ? inferredType : IR::UnsignedType::get(32u, ctx));
  // NOLINTEND(readability-magic-numbers)
}

Json UnsignedLiteral::toJson() const {
  return Json()._("nodeType", "unsignedLiteral")._("value", value)._("fileRange", fileRange);
}

String UnsignedLiteral::toString() const { return value; }

} // namespace qat::ast