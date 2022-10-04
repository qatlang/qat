#include "./unsigned_literal.hpp"

namespace qat::ast {

UnsignedLiteral::UnsignedLiteral(String _value, utils::FileRange _fileRange)
    : Expression(std::move(_fileRange)), value(std::move(_value)) {}

IR::ConstantValue* UnsignedLiteral::emit(IR::Context* ctx) {
  if (getExpectedKind() == ExpressionKind::assignable) {
    ctx->Error("Unsigned literals are not assignable", fileRange);
  }
  // NOLINTBEGIN(readability-magic-numbers)
  return new IR::ConstantValue(llvm::ConstantInt::get(expected
                                                          ? llvm::dyn_cast<llvm::IntegerType>(expected->getLLVMType())
                                                          : llvm::Type::getInt32Ty(ctx->llctx),
                                                      value, 10u),
                               expected ? expected : IR::UnsignedType::get(32u, ctx->llctx));
  // NOLINTEND(readability-magic-numbers)
}

void UnsignedLiteral::setType(IR::QatType* typ) const { expected = typ; }

Json UnsignedLiteral::toJson() const {
  return Json()._("nodeType", "unsignedLiteral")._("value", value)._("fileRange", fileRange);
}

} // namespace qat::ast