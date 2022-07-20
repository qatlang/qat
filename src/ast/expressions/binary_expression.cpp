#include "./binary_expression.hpp"

namespace qat::ast {

IR::Value *BinaryExpression::emit(IR::Context *ctx) {
  // TODO - Implement this
}

nuo::Json BinaryExpression::toJson() const {
  return nuo::Json()
      ._("nodeType", "binaryExpression")
      ._("operator", op)
      ._("lhs", lhs->toJson())
      ._("rhs", rhs->toJson())
      ._("fileRange", fileRange);
}

} // namespace qat::ast