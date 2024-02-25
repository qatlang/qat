#include "./vector.hpp"
#include "../../IR/types/c_type.hpp"
#include "../../IR/types/vector.hpp"
#include "../expression.hpp"
#include "llvm/Analysis/ConstantFolding.h"

namespace qat::ast {

void VectorType::update_dependencies(IR::EmitPhase phase, Maybe<IR::DependType> expect, IR::EntityState* ent,
                                     IR::Context* ctx) {
  subType->update_dependencies(phase, IR::DependType::complete, ent, ctx);
  count->update_dependencies(phase, IR::DependType::complete, ent, ctx);
}

IR::QatType* VectorType::emit(IR::Context* ctx) {
  SHOW("Scalable has value " << scalable.has_value())
  auto subTy       = subType->emit(ctx);
  auto usableSubTy = subTy->isCType() ? subTy->asCType()->getSubType() : subTy;
  if (usableSubTy->isInteger() || usableSubTy->isUnsignedInteger() || usableSubTy->isFloat()) {
    if (count->hasTypeInferrance()) {
      count->asTypeInferrable()->setInferenceType(IR::UnsignedType::get(32u, ctx));
    }
    auto num = count->emit(ctx);
    if (num->getType()->isUnsignedInteger() && num->getType()->asUnsignedInteger()->getBitwidth() == 32u) {
      auto numVal =
          llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldConstant(num->getLLVMConstant(), ctx->dataLayout.value()));
      if (numVal->isZero()) {
        ctx->Error("The number of elements of a vectorized type cannot be 0", count->fileRange);
      }
      return IR::VectorType::create(subTy, *numVal->getValue().getRawData(),
                                    scalable.has_value() ? IR::VectorKind::scalable : IR::VectorKind::fixed, ctx);
    } else {
      ctx->Error("The number of elements of a vectorized type should be of " + ctx->highlightError("u32") + "type",
                 count->fileRange);
    }
  } else {
    ctx->Error(
        "The element type of a vectorized type should be a signed or unsigned integer type, or a floating point type. The element type provided here is " +
            ctx->highlightError(subTy->toString()),
        subType->fileRange);
  }
  return nullptr;
}

Json VectorType::toJson() const {
  return Json()
      ._("typeKind", "vector")
      ._("subType", subType->toJson())
      ._("count", count->toJson())
      ._("isScalable", scalable.has_value())
      ._("fileRange", fileRange);
}

String VectorType::toString() const {
  return String("vec:[") + (scalable.has_value() ? "?, " : "") + count->toString() + ", " + subType->toString() + "]";
}

} // namespace qat::ast