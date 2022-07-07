#include "./binary_expression.hpp"
namespace qat {
namespace AST {

llvm::Value *BinaryExpression::emit(IR::Context *ctx) {
  // TODO - Implement this
}

void BinaryExpression::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (!isHeader) {
    file += "(";
    lhs->emitCPP(file, isHeader);
    file += (" " + op + " ");
    rhs->emitCPP(file, isHeader);
    file += ")";
  }
}

backend::JSON BinaryExpression::toJSON() const {
  return backend::JSON()
      ._("nodeType", "binaryExpression")
      ._("operator", op)
      ._("lhs", lhs->toJSON())
      ._("rhs", rhs->toJSON())
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat