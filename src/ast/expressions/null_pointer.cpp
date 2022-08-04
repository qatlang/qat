#include "./null_pointer.hpp"

namespace qat::ast {

NullPointer::NullPointer(utils::FileRange _fileRange)
    : type(nullptr), Expression(std::move(_fileRange)) {}

NullPointer::NullPointer(llvm::Type *_type, utils::FileRange _fileRange)
    : type(_type), Expression(std::move(_fileRange)) {}

IR::Value *NullPointer::emit(IR::Context *ctx) {
  if (getExpectedKind() == ExpressionKind::assignable) {
    ctx->Error("Null pointer is not assignable", fileRange);
  }
  // TODO - Implement this
}

nuo::Json NullPointer::toJson() const {
  return nuo::Json()._("nodeType", "nullPointer");
}

} // namespace qat::ast