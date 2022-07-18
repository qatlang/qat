#include "./binary_expression.hpp"

namespace qat::AST {

IR::Value *BinaryExpression::emit(IR::Context *ctx) {
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

nuo::Json BinaryExpression::toJson() const {
  return nuo::Json()
      ._("nodeType", "binaryExpression")
      ._("operator", op)
      ._("lhs", lhs->toJson())
      ._("rhs", rhs->toJson())
      ._("filePlacement", fileRange);
}

} // namespace qat::AST