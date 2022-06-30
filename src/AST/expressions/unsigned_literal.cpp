#include "./unsigned_literal.hpp"

namespace qat {
namespace AST {

UnsignedLiteral::UnsignedLiteral(std::string _value,
                                 utils::FilePlacement _filePlacement)
    : value(_value), Expression(_filePlacement) {}

llvm::Value *UnsignedLiteral::generate(IR::Generator *generator) {
  if (getExpectedKind() == ExpressionKind::assignable) {
    generator->throw_error("This expression is not assignable", file_placement);
  }
  return llvm::ConstantInt::get(llvm::Type::getInt32Ty(generator->llvmContext),
                                llvm::StringRef(value), 10u);
}

} // namespace AST
} // namespace qat