#include "./array.hpp"
#include "../../IR/types/array.hpp"
#include "../expression.hpp"
#include "llvm/IR/DerivedTypes.h"

namespace qat::ast {

#define ARRAY_LENGTH_BITWIDTH 64u

void ArrayType::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
                                    EmitCtx* ctx) {
  lengthExp->update_dependencies(phase, ir::DependType::complete, ent, ctx);
  elementType->update_dependencies(phase, ir::DependType::complete, ent, ctx);
}

void ArrayType::typeInferenceForLength(ir::Ctx* irCtx) const {
  if (lengthExp->has_type_inferrance()) {
    lengthExp->as_type_inferrable()->set_inference_type(ir::UnsignedType::get(ARRAY_LENGTH_BITWIDTH, irCtx));
  }
}

Maybe<usize> ArrayType::getTypeSizeInBits(EmitCtx* ctx) const {
  auto elemSize = elementType->getTypeSizeInBits(ctx);
  typeInferenceForLength(ctx->irCtx);
  auto* lengthIR = lengthExp->emit(ctx);
  if (lengthIR->get_ir_type()->is_unsigned_integer() &&
      (lengthIR->get_ir_type()->as_unsigned_integer()->getBitwidth() <= ARRAY_LENGTH_BITWIDTH)) {
    auto length = *llvm::cast<llvm::ConstantInt>(lengthIR->get_llvm_constant())->getValue().getRawData();
    if (elemSize.has_value() && (length > 0u)) {
      return (usize)(ctx->mod->get_llvm_module()->getDataLayout().getTypeAllocSizeInBits(
          llvm::ArrayType::get(llvm::Type::getIntNTy(ctx->irCtx->llctx, elemSize.value()), length)));
    } else {
      return None;
    }
  } else {
    return None;
  }
}

ir::Type* ArrayType::emit(EmitCtx* ctx) {
  typeInferenceForLength(ctx->irCtx);
  auto* lengthIR = lengthExp->emit(ctx);
  if (lengthIR->get_ir_type()->is_unsigned_integer()) {
    if (lengthIR->get_ir_type()->as_unsigned_integer()->getBitwidth() > ARRAY_LENGTH_BITWIDTH) {
      ctx->Error(
          "Length of an array is an unsigned integer with bitwidth of " +
              ctx->color(std::to_string(lengthIR->get_ir_type()->as_unsigned_integer()->getBitwidth())) +
              " which is not allowed. Bitwidth of the length of the array should be less than or equal to 64, as there can be loss of data during compilation otherwise",
          lengthExp->fileRange);
    }
    return ir::ArrayType::get(elementType->emit(ctx),
                              *llvm::cast<llvm::ConstantInt>(lengthIR->get_llvm_constant())->getValue().getRawData(),
                              ctx->irCtx->llctx);
  } else {
    ctx->Error("Length of an array should be of unsigned integer type, but the provided value is of " +
                   ctx->color(lengthIR->get_ir_type()->to_string()),
               fileRange);
  }
}

AstTypeKind ArrayType::type_kind() const { return AstTypeKind::ARRAY; }

Json ArrayType::to_json() const {
  return Json()
      ._("typeKind", "array")
      ._("subType", elementType->to_json())
      ._("length", lengthExp->to_json())
      ._("fileRange", fileRange);
}

String ArrayType::to_string() const { return "[" + lengthExp->to_string() + "]" + elementType->to_string(); }

} // namespace qat::ast