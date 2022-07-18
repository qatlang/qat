#include "./null_pointer.hpp"

namespace qat::ast {

NullPointer::NullPointer(utils::FileRange _fileRange)
    : type(nullptr), Expression(_fileRange) {}

NullPointer::NullPointer(llvm::Type *_type, utils::FileRange _fileRange)
    : type(_type), Expression(_fileRange) {}

IR::Value *NullPointer::emit(IR::Context *ctx) {
  if (getExpectedKind() == ExpressionKind::assignable) {
    ctx->throw_error("Null pointer is not assignable", fileRange);
  }
  // TODO - Implement this
}

void NullPointer::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (!isHeader) {
    file += "nullptr";
  }
}

nuo::Json NullPointer::toJson() const {
  return nuo::Json()._("nodeType", "nullPointer");
}

} // namespace qat::ast