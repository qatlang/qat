#include "./assignment.hpp"

namespace qat::ast {

Assignment::Assignment(Expression *_lhs, Expression *_value,
                       utils::FileRange _fileRange)
    : Sentence(_fileRange), lhs(_lhs), value(_value) {}

IR::Value *Assignment::emit(IR::Context *ctx) {
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
      ._("fileRange", fileRange);
}

} // namespace qat::ast
