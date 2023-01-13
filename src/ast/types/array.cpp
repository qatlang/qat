#include "./array.hpp"
#include "../../IR/types/array.hpp"
#include "llvm/IR/DerivedTypes.h"

namespace qat::ast {

ArrayType::ArrayType(QatType* _element_type, u64 _length, bool _variable, FileRange _fileRange)
    : QatType(_variable, std::move(_fileRange)), elementType(_element_type), length(_length) {}

Maybe<usize> ArrayType::getTypeSizeInBits(IR::Context* ctx) const {
  auto elemSize = elementType->getTypeSizeInBits(ctx);
  if (elemSize.has_value() && (length > 0u)) {
    return (usize)(ctx->getMod()->getLLVMModule()->getDataLayout().getTypeAllocSizeInBits(
        llvm::ArrayType::get(llvm::Type::getIntNTy(ctx->llctx, elemSize.value()), length)));
  } else {
    return None;
  }
}

IR::QatType* ArrayType::emit(IR::Context* ctx) {
  return IR::ArrayType::get(elementType->emit(ctx), length, ctx->llctx);
}

TypeKind ArrayType::typeKind() const { return TypeKind::array; }

Json ArrayType::toJson() const {
  return Json()
      ._("typeKind", "array")
      ._("subType", elementType->toJson())
      ._("isVariable", isVariable())
      ._("fileRange", fileRange);
}

String ArrayType::toString() const {
  return (isVariable() ? "var " : "") + elementType->toString() + "[" + std::to_string(length) + "]";
}

} // namespace qat::ast