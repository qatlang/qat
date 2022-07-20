#include "./self.hpp"

namespace qat::ast {

Self::Self(utils::FileRange _fileRange) : Expression(_fileRange) {}

IR::Value *Self::emit(IR::Context *ctx) {
  if (getExpectedKind() == ExpressionKind::assignable) {
    ctx->throw_error("Self is not assignable", fileRange);
  }
  // TODO - Implement this
}

nuo::Json Self::toJson() const {
  return nuo::Json()._("nodeType", "selfExpression")._("fileRange", fileRange);
}

} // namespace qat::ast