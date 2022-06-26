#include "./integer_literal.hpp"

namespace qat {
namespace AST {

IntegerLiteral::IntegerLiteral(std::string _value, bool _isUnsigned,
                               utils::FilePlacement _filePlacement)
    : value(_value), isUnsigned(_isUnsigned), Expression(_filePlacement) {}

llvm::Value *IntegerLiteral::generate(IR::Generator *generator) {
  if (getExpectedKind() == ExpressionKind::assignable) {
    generator->throw_error("This expression is not assignable", file_placement);
  }
  auto type = llvm::Type::getInt64Ty(generator->llvmContext);
  // FIXME - Support custom radix
  auto result = llvm::ConstantInt::get(type, llvm::StringRef(value), 10u);
  // FIXME - Support unsigned integer literals
  return result;
}

} // namespace AST
} // namespace qat