#include "./size_of.hpp"

namespace qat {
namespace AST {

SizeOf::SizeOf(Expression *_expression, utils::FilePlacement _filePlacement)
    : expression(_expression), Expression(_filePlacement) {}

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
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat