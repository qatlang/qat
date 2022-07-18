#include "./size_of.hpp"

namespace qat::ast {

SizeOf::SizeOf(Expression *_expression, utils::FileRange _fileRange)
    : expression(_expression), Expression(_fileRange) {}

IR::Value *SizeOf::emit(IR::Context *ctx) {
  // TODO - Implement this
}

void SizeOf::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (!isHeader) {
    file += "sizeof(";
    expression->emitCPP(file, isHeader);
    file += ")";
  }
}

nuo::Json SizeOf::toJson() const {
  return nuo::Json()
      ._("nodeType", "sizeOf")
      ._("expression", expression->toJson())
      ._("fileRange", fileRange);
}

} // namespace qat::ast