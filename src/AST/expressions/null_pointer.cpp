#include "./null_pointer.hpp"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Type.h"

namespace qat {
namespace AST {

NullPointer::NullPointer(utils::FilePlacement _filePlacement)
    : type(nullptr), Expression(_filePlacement) {}

NullPointer::NullPointer(llvm::Type *_type, utils::FilePlacement _filePlacement)
    : type(_type), Expression(_filePlacement) {}

llvm::Value *NullPointer::emit(IR::Context *ctx) {
  if (getExpectedKind() == ExpressionKind::assignable) {
    ctx->throw_error("Null pointer is not assignable", file_placement);
  };
  return llvm::ConstantPointerNull::get(
      type ? type->getPointerTo() : llvm::Type::getInt8PtrTy(ctx->llvmContext));
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