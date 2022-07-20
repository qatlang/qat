#include "./assignment.hpp"

namespace qat::ast {

Assignment::Assignment(Expression *_lhs, Expression *_value,
                       utils::FileRange _fileRange)
    : Sentence(_fileRange), lhs(_lhs), value(_value) {}

IR::Value *Assignment::emit(IR::Context *ctx) {
  // TODO - Implement this
}

nuo::Json Assignment::toJson() const {
  return nuo::Json()
      ._("nodeType", "assignment")
      ._("lhs", lhs->toJson())
      ._("rhs", value->toJson())
      ._("fileRange", fileRange);
}

} // namespace qat::ast
