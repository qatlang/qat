#include "./array.hpp"
#include "../../IR/types/array.hpp"
#include "../constants/integer_literal.hpp"
#include "../expression.hpp"
#include "llvm/IR/DerivedTypes.h"

namespace qat::ast {

#define ARRAY_LENGTH_BITWIDTH 64u

ArrayType::ArrayType(QatType* _element_type, ConstantExpression* _length, bool _variable, FileRange _fileRange)
    : QatType(_variable, std::move(_fileRange)), elementType(_element_type), lengthExp(_length) {}

void ArrayType::typeInferenceForLength(llvm::LLVMContext& llCtx) const {
  if (lengthExp->nodeType() == NodeType::integerLiteral) {
    ((IntegerLiteral*)lengthExp)->setType(IR::UnsignedType::get(ARRAY_LENGTH_BITWIDTH, llCtx));
  }
}

Maybe<usize> ArrayType::getTypeSizeInBits(IR::Context* ctx) const {
  auto elemSize = elementType->getTypeSizeInBits(ctx);
  typeInferenceForLength(ctx->llctx);
  auto* lengthIR = lengthExp->emit(ctx);
  if (lengthIR->getType()->isUnsignedInteger()) {
    auto length = *llvm::cast<llvm::ConstantInt>(lengthIR->getLLVMConstant())->getValue().getRawData();
    if (elemSize.has_value() && (length > 0u)) {
      return (usize)(ctx->getMod()->getLLVMModule()->getDataLayout().getTypeAllocSizeInBits(
          llvm::ArrayType::get(llvm::Type::getIntNTy(ctx->llctx, elemSize.value()), length)));
    } else {
      return None;
    }
  } else {
    return None;
  }
}

IR::QatType* ArrayType::emit(IR::Context* ctx) {
  typeInferenceForLength(ctx->llctx);
  auto* lengthIR = lengthExp->emit(ctx);
  if (lengthIR->getType()->isUnsignedInteger()) {
    return IR::ArrayType::get(elementType->emit(ctx),
                              *llvm::cast<llvm::ConstantInt>(lengthIR->getLLVMConstant())->getValue().getRawData(),
                              ctx->llctx);
  } else {
    ctx->Error("Length of an array should be of unsigned integer type, but the provided value is of " +
                   ctx->highlightError(lengthIR->getType()->toString()),
               fileRange);
  }
}

TypeKind ArrayType::typeKind() const { return TypeKind::array; }

Json ArrayType::toJson() const {
  return Json()
      ._("typeKind", "array")
      ._("subType", elementType->toJson())
      ._("length", lengthExp->toJson())
      ._("isVariable", isVariable())
      ._("fileRange", fileRange);
}

String ArrayType::toString() const {
  return (isVariable() ? "var " : "") + elementType->toString() + "[" + lengthExp->toString() + "]";
}

} // namespace qat::ast