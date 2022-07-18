#include "./self.hpp"

namespace qat::AST {

Self::Self(utils::FileRange _filePlacement) : Expression(_filePlacement) {}

IR::Value *Self::emit(IR::Context *ctx) {
  if (getExpectedKind() == ExpressionKind::assignable) {
    ctx->throw_error("Self is not assignable", fileRange);
  }
  // TODO - Implement this
}

void Self::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (!isHeader) {
    file += "this";
  }
}

nuo::Json Self::toJson() const {
  return nuo::Json()
      ._("nodeType", "selfExpression")
      ._("filePlacement", fileRange);
}

} // namespace qat::AST