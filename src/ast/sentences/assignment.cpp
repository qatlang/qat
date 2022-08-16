#include "./assignment.hpp"

namespace qat::ast {

Assignment::Assignment(Expression *_lhs, Expression *_value,
                       utils::FileRange _fileRange)
    : Sentence(std::move(_fileRange)), lhs(_lhs), value(_value) {}

IR::Value *Assignment::emit(IR::Context *ctx) {
  lhs->setExpectedKind(ExpressionKind::assignable);
  auto *lhsVal = lhs->emit(ctx);
  auto *expVal = value->emit(ctx);
  if (lhsVal->isVariable()) {
    if (lhsVal->isImplicitPointer() || lhsVal->getType()->isReference() ||
        lhsVal->getType()->isPointer() ||
        lhsVal->getLLVM()->getType()->isPointerTy()) {
      auto *lhsType = lhsVal->getType();
      auto *expType = expVal->getType();
      if (lhsType->isSame(expType) ||
          (lhsVal->isReference() &&
           lhsType->asReference()->getSubType()->isSame(expType)) ||
          (lhsVal->isPointer() &&
           lhsType->asPointer()->getSubType()->isSame(expType)) ||
          (expType->isReference() &&
           expType->asReference()->getSubType()->isSame(lhsType))) {
        if (expType->isReference() || expVal->isImplicitPointer()) {
          SHOW("Expression for assignment is of type "
               << expType->asReference()->getSubType()->toString())
          expVal = new IR::Value(
              ctx->builder.CreateLoad(
                  expType->isReference()
                      ? expType->asReference()->getSubType()->getLLVMType()
                      : expType->getLLVMType(),
                  expVal->getLLVM()),
              expVal->getType(), expVal->isVariable(), expVal->getNature());
        }
        ctx->builder.CreateStore(expVal->getLLVM(), lhsVal->getLLVM());
        return nullptr;
      } else {
        ctx->Error("Type of the left hand side and the right hand side of the "
                   "assignment do not match. Please check the logic.",
                   fileRange);
      }
    } else {
      ctx->Error("Left hand side of the assignment cannot be assigned to",
                 fileRange);
    }
  } else {
    ctx->Error("Left hand side of the assignment cannot be assigned to because "
               "it is not a variable value",
               fileRange);
  }
}

nuo::Json Assignment::toJson() const {
  return nuo::Json()
      ._("nodeType", "assignment")
      ._("lhs", lhs->toJson())
      ._("rhs", value->toJson())
      ._("fileRange", fileRange);
}

} // namespace qat::ast
