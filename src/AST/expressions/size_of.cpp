#include "./size_of.hpp"

namespace qat {
namespace AST {

SizeOf::SizeOf(Expression *_expression, utils::FilePlacement _filePlacement)
    : expression(_expression), Expression(_filePlacement) {}

llvm::Value *SizeOf::emit(IR::Context *ctx) {
  auto gen_exp = expression->emit(ctx);
  return llvm::ConstantExpr::getSizeOf(gen_exp->getType());
}

void SizeOf::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (!isHeader) {
    file += "sizeof(";
    expression->emitCPP(file, isHeader);
    file += ")";
  }
}

backend::JSON SizeOf::toJSON() const {
  return backend::JSON()
      ._("nodeType", "sizeOf")
      ._("expression", expression->toJSON())
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat