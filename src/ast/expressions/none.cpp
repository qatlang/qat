#include "./none.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/maybe.hpp"

namespace qat::ast {

NoneExpression::NoneExpression(QatType* _type, utils::FileRange _fileRange)
    : Expression(std::move(_fileRange)), type(_type) {}

void NoneExpression::setType(IR::QatType* _irType) { irType = _irType; }

IR::Value* NoneExpression::emit(IR::Context* ctx) {
  if (type || irType) {
    auto* typ = type ? type->emit(ctx) : irType;
    if (irType && type) {
      if (!typ->isSame(irType)) {
        ctx->Error("The type provided for this none expression is " + ctx->highlightError(typ->toString()) +
                       ", but the expected type is " + ctx->highlightError(irType->toString()),
                   fileRange);
      }
    }
    SHOW("Type for none expression is: " << typ->toString())
    auto* mTy    = IR::MaybeType::get(typ, ctx->llctx);
    auto* newVal = IR::Logic::newAlloca(ctx->fn, utils::unique_id(), mTy->getLLVMType());
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
      ._("type", (type != nullptr ? type->toJson() : Json()))
      ._("fileRange", fileRange);
}

} // namespace qat::ast