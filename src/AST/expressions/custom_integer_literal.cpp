#include "./custom_integer_literal.hpp"

namespace qat {
namespace AST {

CustomIntegerLiteral::CustomIntegerLiteral(std::string _value, bool _isUnsigned,
                                           unsigned int _bitWidth,
                                           utils::FilePlacement _filePlacement)
    : value(_value), isUnsigned(_isUnsigned), bitWidth(_bitWidth),
      Expression(_filePlacement) {}

llvm::Value *CustomIntegerLiteral::generate(IR::Generator *generator) {
  if (isExpectedKind(ExpressionKind::assignable)) {
    generator->throw_error("This expression is not assignable", file_placement);
  }
  return llvm::ConstantInt::get(
      llvm::Type::getIntNTy(generator->llvmContext, bitWidth), value, 10u);
}

backend::JSON CustomIntegerLiteral::toJSON() const {
  return backend::JSON()
      ._("nodeType", "customIntegerLiteral")
      ._("isUnsigned", isUnsigned)
      ._("bitWidth", (unsigned long long)bitWidth)
      ._("value", value)
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat