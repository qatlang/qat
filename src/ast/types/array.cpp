#include "./array.hpp"
#include "../../IR/types/array.hpp"
#include "../expression.hpp"
#include "llvm/IR/DerivedTypes.h"

namespace qat::ast {

#define ARRAY_LENGTH_BITWIDTH 64u

void ArrayType::update_dependencies(IR::EmitPhase phase, Maybe<IR::DependType> expect, IR::EntityState* ent,
                                    IR::Context* ctx) {
  lengthExp->update_dependencies(phase, IR::DependType::complete, ent, ctx);
  elementType->update_dependencies(phase, IR::DependType::complete, ent, ctx);
}

void ArrayType::typeInferenceForLength(IR::Context* ctx) const {
  if (lengthExp->hasTypeInferrance()) {
    lengthExp->asTypeInferrable()->setInferenceType(IR::UnsignedType::get(ARRAY_LENGTH_BITWIDTH, ctx));
  }
}

Maybe<usize> ArrayType::getTypeSizeInBits(IR::Context* ctx) const {
  auto elemSize = elementType->getTypeSizeInBits(ctx);
  typeInferenceForLength(ctx);
  auto* lengthIR = lengthExp->emit(ctx);
  if (lengthIR->getType()->isUnsignedInteger() &&
      (lengthIR->getType()->asUnsignedInteger()->getBitwidth() <= ARRAY_LENGTH_BITWIDTH)) {
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
  typeInferenceForLength(ctx);
  auto* lengthIR = lengthExp->emit(ctx);
  if (lengthIR->getType()->isUnsignedInteger()) {
    if (lengthIR->getType()->asUnsignedInteger()->getBitwidth() > ARRAY_LENGTH_BITWIDTH) {
      ctx->Error(
          "Length of an array is an unsigned integer with bitwidth of " +
              ctx->highlightError(std::to_string(lengthIR->getType()->asUnsignedInteger()->getBitwidth())) +
              " which is not allowed. Bitwidth of the length of the array should be less than or equal to 64, as there can be loss of data during compilation otherwise",
          lengthExp->fileRange);
    }
    return IR::ArrayType::get(elementType->emit(ctx),
                              *llvm::cast<llvm::ConstantInt>(lengthIR->getLLVMConstant())->getValue().getRawData(),
                              ctx->llctx);
  } else {
    ctx->Error("Length of an array should be of unsigned integer type, but the provided value is of " +
                   ctx->highlightError(lengthIR->getType()->toString()),
               fileRange);
  }
}

AstTypeKind ArrayType::typeKind() const { return AstTypeKind::ARRAY; }

Json ArrayType::toJson() const {
  return Json()
      ._("typeKind", "array")
      ._("subType", elementType->toJson())
      ._("length", lengthExp->toJson())
      ._("fileRange", fileRange);
}

String ArrayType::toString() const { return "[" + lengthExp->toString() + "]" + elementType->toString(); }

} // namespace qat::ast