#include "./none.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/maybe.hpp"

namespace qat::ast {

IR::Value* NoneExpression::emit(IR::Context* ctx) {
  if (type || isTypeInferred()) {
    if (isTypeInferred()) {
      if (!inferredType->isMaybe()) {
        ctx->Error("The inferred type of the " + ctx->highlightError("none") + " expression is " +
                       ctx->highlightError(inferredType->toString()) + " which is not a maybe type",
                   fileRange);
      } else if (type && isPacked.has_value() && inferredType->isMaybe() &&
                 (!inferredType->asMaybe()->isTypePacked())) {
        ctx->Error("The inferred maybe type is " + ctx->highlightError(inferredType->toString()) +
                       " which is not packed, but " + ctx->highlightError("pack") +
                       " is provided for the none expression. Please change this to " +
                       ctx->highlightError("none:[" + type->toString() + "]"),
                   isPacked.value());
      } else if (type && !isPacked.has_value() && inferredType->isMaybe() && inferredType->asMaybe()->isTypePacked()) {
        ctx->Error("The inferred maybe type is " + ctx->highlightError(inferredType->toString()) +
                       " which is packed, but " + ctx->highlightError("pack") +
                       " is not provided in the none expression. Please change this to " +
                       ctx->highlightError("none:[pack, " + type->toString() + "]"),
                   fileRange);
      }
    }
    auto* typ = type ? type->emit(ctx) : inferredType->asMaybe()->getSubType();
    if (inferredType && type) {
      if (!typ->isSame(inferredType->asMaybe()->getSubType())) {
        ctx->Error("The type provided for this none expression is " + ctx->highlightError(typ->toString()) +
                       ", but the expected type is " + ctx->highlightError(inferredType->asMaybe()->toString()),
                   fileRange);
      }
    }
    SHOW("Type for none expression is: " << typ->toString())
    auto* mTy    = IR::MaybeType::get(typ, isPacked.has_value(), ctx);
    auto* newVal = IR::Logic::newAlloca(ctx->getActiveFunction(), None, mTy->getLLVMType());
    ctx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 0u),
                             ctx->builder.CreateStructGEP(mTy->getLLVMType(), newVal, 0u));
    ctx->builder.CreateStore(llvm::Constant::getNullValue(mTy->getSubType()->getLLVMType()),
                             ctx->builder.CreateStructGEP(mTy->getLLVMType(), newVal, 1u));
    return new IR::Value(newVal, mTy, false, IR::Nature::temporary);
  } else {
    ctx->Error("No type found for the none expression. A type is required to be able to create the none value",
               fileRange);
  }
  return nullptr;
}

Json NoneExpression::toJson() const {
  return Json()
      ._("nodeType", "none")
      ._("hasType", (type != nullptr))
      ._("isPacked", isPacked.has_value())
      ._("packRange", isPacked.has_value() ? isPacked.value() : JsonValue())
      ._("type", (type != nullptr ? type->toJson() : Json()))
      ._("fileRange", fileRange);
}

} // namespace qat::ast