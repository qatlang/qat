#include "./give_sentence.hpp"
namespace qat::ast {

GiveSentence::GiveSentence(std::optional<Expression *> _given_expr,
                           utils::FileRange _fileRange)
    : Sentence(_fileRange), give_expr(_given_expr) {}

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

nuo::Json GiveSentence::toJson() const {
  return nuo::Json()
      ._("nodeType", "giveSentence")
      ._("hasValue", give_expr.has_value())
      ._("value",
         give_expr.has_value() ? give_expr.value()->toJson() : nuo::Json())
      ._("fileRange", fileRange);
}

} // namespace qat::ast