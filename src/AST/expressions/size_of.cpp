#include "./size_of.hpp"

namespace qat {
namespace AST {

SizeOf::SizeOf(Expression *_expression, utils::FilePlacement _filePlacement)
    : expression(_expression), Expression(_filePlacement) {}

llvm::Value *SizeOf::generate(IR::Generator *generator) {
  auto gen_exp = expression->generate(generator);
  return llvm::ConstantExpr::getSizeOf(gen_exp->getType());
}

backend::JSON SizeOf::toJSON() const {
  return backend::JSON()
      ._("nodeType", "sizeOf")
      ._("expression", expression->toJSON())
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat