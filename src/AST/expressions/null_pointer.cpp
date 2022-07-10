#include "./null_pointer.hpp"

namespace qat {
namespace AST {

NullPointer::NullPointer(utils::FilePlacement _filePlacement)
    : type(nullptr), Expression(_filePlacement) {}

NullPointer::NullPointer(llvm::Type *_type, utils::FilePlacement _filePlacement)
    : type(_type), Expression(_filePlacement) {}

IR::Value *NullPointer::emit(IR::Context *ctx) {
  if (getExpectedKind() == ExpressionKind::assignable) {
    ctx->throw_error("Null pointer is not assignable", file_placement);
  };
  // TODO - Implement this
}

void NullPointer::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (!isHeader) {
    file += "nullptr";
  }
}

backend::JSON NullPointer::toJSON() const {
  return backend::JSON()._("nodeType", "nullPointer");
}

} // namespace AST
} // namespace qat