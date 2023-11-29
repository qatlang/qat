#include "./integer_literal.hpp"

namespace qat::ast {

IntegerLiteral::IntegerLiteral(String _value, FileRange _fileRange)
    : PrerunExpression(std::move(_fileRange)), value(std::move(_value)) {}

IR::PrerunValue* IntegerLiteral::emit(IR::Context* ctx) {
  if (isTypeInferred() && (!inferredType->isInteger() && !inferredType->isUnsignedInteger())) {
    ctx->Error("The inferred type of this expression is " + inferredType->toString() +
                   ". The only supported types in type inference for integer literal are signed & unsigned integers",
               fileRange);
  }
  String intValue = value;
  if (value.find('_') != String::npos) {
    intValue = "";
    for (auto intCh : value) {
      if (intCh != '_') {
        intValue += intCh;
      }
    }
  }
  // NOLINTBEGIN(readability-magic-numbers)
  return new IR::PrerunValue(llvm::ConstantInt::get(isTypeInferred()
                                                        ? llvm::cast<llvm::IntegerType>(inferredType->getLLVMType())
                                                        : llvm::Type::getInt32Ty(ctx->llctx),
                                                    intValue, 10u),
                             isTypeInferred() ? inferredType : IR::IntegerType::get(32, ctx));
  // NOLINTEND(readability-magic-numbers)
}

String IntegerLiteral::toString() const { return value; }

Json IntegerLiteral::toJson() const {
  return Json()._("nodeType", "integerLiteral")._("value", value)._("fileRange", fileRange);
}

} // namespace qat::ast