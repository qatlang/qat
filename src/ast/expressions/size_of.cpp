#include "./size_of.hpp"

namespace qat::ast {

SizeOf::SizeOf(Expression *_expression, utils::FileRange _fileRange)
    : expression(_expression), Expression(_fileRange) {}

IR::Value *SizeOf::emit(IR::Context *ctx) {
  // TODO - Implement this
}

nuo::Json SizeOf::toJson() const {
  return nuo::Json()
      ._("nodeType", "sizeOf")
      ._("expression", expression->toJson())
      ._("fileRange", fileRange);
}

} // namespace qat::ast