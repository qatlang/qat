#include "./assignment.hpp"

namespace qat {
namespace AST {

Assignment::Assignment(Expression *_lhs, Expression *_value,
                       utils::FilePlacement _filePlacement)
    : lhs(_lhs), value(_value), Sentence(_filePlacement) {}

IR::Value *Assignment::emit(qat::IR::Context *ctx) {
  // TODO - Implement this
}

void Assignment::emitCPP(backend::cpp::File &file, bool isHeader) const {
  lhs->emitCPP(file, isHeader);
  file += " = ";
  value->emitCPP(file, isHeader);
  file += ";\n";
}

backend::JSON Assignment::toJSON() const {
  return backend::JSON()
      ._("nodeType", "assignment")
      ._("lhs", lhs->toJSON())
      ._("rhs", value->toJSON())
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat
