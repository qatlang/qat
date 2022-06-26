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
  auto type = llvm::Type::getIntNTy(generator->llvmContext, bitWidth);
  // FIXME - Support custom radix
  auto result = llvm::ConstantInt::get(type, llvm::StringRef(value), 10u);
  // FIXME - Support unsigned integer literals
  return result;
}

} // namespace AST
} // namespace qat