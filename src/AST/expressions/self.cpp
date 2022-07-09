#include "./self.hpp"

namespace qat {
namespace AST {

Self::Self(utils::FilePlacement _filePlacement) : Expression(_filePlacement) {}

IR::Value *Self::emit(IR::Context *ctx) {
  if (getExpectedKind() == ExpressionKind::assignable) {
    ctx->throw_error("Self is not assignable", file_placement);
  }
  // TODO - Implement this
}

void Self::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (!isHeader) {
    file += "this";
  }
}

backend::JSON Self::toJSON() const {
  return backend::JSON()
      ._("nodeType", "selfExpression")
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat