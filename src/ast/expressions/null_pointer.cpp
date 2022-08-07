#include "./null_pointer.hpp"
#include "llvm/IR/Constants.h"

namespace qat::ast {

NullPointer::NullPointer(utils::FileRange _fileRange)
    : Expression(std::move(_fileRange)), type(nullptr) {}

NullPointer::NullPointer(llvm::Type *_type, utils::FileRange _fileRange)
    : Expression(std::move(_fileRange)), type(_type) {}

IR::Value *NullPointer::emit(IR::Context *ctx) {
  if (getExpectedKind() == ExpressionKind::assignable) {
    ctx->Error("Null pointer is not assignable", fileRange);
  }
  return new IR::Value(
      llvm::ConstantPointerNull::get(
          llvm::Type::getInt8Ty(ctx->llctx)->getPointerTo()),
      IR::PointerType::get(IR::VoidType::get(ctx->llctx), ctx->llctx), false,
      IR::Nature::pure);
}

nuo::Json NullPointer::toJson() const {
  return nuo::Json()._("nodeType", "nullPointer");
}

} // namespace qat::ast