#include "./integer_literal.hpp"

namespace qat {
namespace AST {

IntegerLiteral::IntegerLiteral(std::string _value,
                               utils::FilePlacement _filePlacement)
    : value(_value), Expression(_filePlacement) {}

llvm::Value *IntegerLiteral::generate(IR::Generator *generator) {
  if (getExpectedKind() == ExpressionKind::assignable) {
    generator->throw_error("This expression is not assignable", file_placement);
  }
  return llvm::ConstantInt::get(llvm::Type::getInt32Ty(generator->llvmContext),
                                value, 10u);
}

} // namespace AST
} // namespace qat