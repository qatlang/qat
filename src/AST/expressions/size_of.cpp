#include "./size_of.hpp"

namespace qat {
namespace AST {

SizeOf::SizeOf(Expression _expression, utils::FilePlacement _filePlacement)
    : expression(_expression), Expression(_filePlacement) {}

llvm::Value *SizeOf::generate(IR::Generator *generator) {
  auto gen_exp = expression.generate(generator);
  return llvm::ConstantExpr::getSizeOf(gen_exp->getType());
}

} // namespace AST
} // namespace qat