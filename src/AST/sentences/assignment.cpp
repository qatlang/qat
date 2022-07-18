#include "./assignment.hpp"

namespace qat::AST {

Assignment::Assignment(Expression *_lhs, Expression *_value,
                       utils::FileRange _filePlacement)
    : Sentence(_filePlacement), lhs(_lhs), value(_value) {}

IR::Value *Assignment::emit(qat::IR::Context *ctx) {
  // TODO - Implement this
}

void Assignment::emitCPP(backend::cpp::File &file, bool isHeader) const {
  lhs->emitCPP(file, isHeader);
  file += " = ";
  value->emitCPP(file, isHeader);
  file += ";\n";
}

nuo::Json Assignment::toJson() const {
  return nuo::Json()
      ._("nodeType", "assignment")
      ._("lhs", lhs->toJson())
      ._("rhs", value->toJson())
      ._("filePlacement", fileRange);
}

} // namespace qat::AST
