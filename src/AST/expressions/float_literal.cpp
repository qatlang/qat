#include "./float_literal.hpp"

namespace qat {
namespace AST {

FloatLiteral::FloatLiteral(std::string _value,
                           utils::FilePlacement _filePlacement)
    : value(_value), Expression(_filePlacement) {}

llvm::Value *FloatLiteral::generate(IR::Generator *generator) {
  if (isExpectedKind(ExpressionKind::assignable)) {
    generator->throw_error("This expression is not assignable", file_placement);
  }
  return llvm::ConstantFP::get(llvm::Type::getFloatTy(generator->llvmContext),
                               llvm::StringRef(value) //
  );
}

} // namespace AST
} // namespace qat