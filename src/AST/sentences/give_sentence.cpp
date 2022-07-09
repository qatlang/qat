#include "./give_sentence.hpp"
namespace qat {
namespace AST {

GiveSentence::GiveSentence(std::optional<Expression *> _given_expr,
                           utils::FilePlacement _filePlacement)
    : give_expr(_given_expr), Sentence(_filePlacement) {}

IR::Value *GiveSentence::emit(IR::Context *ctx) {
  // TODO - Implement this
}

void GiveSentence::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (!isHeader) {
    file += "return ";
    if (give_expr.has_value()) {
      give_expr.value()->emitCPP(file, isHeader);
    }
    file += ";";
  }
}

backend::JSON GiveSentence::toJSON() const {
  return backend::JSON()
      ._("nodeType", "giveSentence")
      ._("hasValue", give_expr.has_value())
      ._("value",
         give_expr.has_value() ? give_expr.value()->toJSON() : backend::JSON())
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat