#include "./assignment.hpp"

namespace qat::ast {

Assignment::Assignment(Expression *_lhs, Expression *_value,
                       utils::FileRange _fileRange)
    : Sentence(std::move(_fileRange)), lhs(_lhs), value(_value) {}

IR::Value *Assignment::emit(IR::Context *ctx) {
  auto *lhsVal = lhs->emit(ctx);
  auto *expVal = value->emit(ctx);
  SHOW("Emitted lhs and rhs of Assignment")
  if (lhsVal->isVariable() ||
      (lhsVal->getType()->isReference() &&
       lhsVal->getType()->asReference()->isSubtypeVariable())) {
    SHOW("Is variable nature")
    if (lhsVal->isImplicitPointer() || lhsVal->getType()->isReference()) {
      SHOW("Getting IR types")
      auto *lhsType = lhsVal->getType();
      auto *expType = expVal->getType();
      if (lhsType->isSame(expType) ||
          (lhsType->isReference() &&
           lhsType->asReference()->getSubType()->isSame(expType)) ||
          (expType->isReference() &&
           expType->asReference()->getSubType()->isSame(lhsType))) {
        SHOW("The general types are the same")
        if (lhsVal->isImplicitPointer() &&
            (lhsType->isReference() || lhsType->isPointer())) {
          SHOW("LHS is implicit pointer")
          lhsVal->loadImplicitPointer(ctx->builder);
        }
        SHOW("Loaded implicit pointer")
        if (expType->isReference() ||
            (!lhsVal->isReference() && expVal->isImplicitPointer())) {
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
        SHOW("Creating store")
        if (expVal->getType()->isReference() || expVal->isImplicitPointer()) {
          SHOW("Loading reference exp")
          ctx->builder.CreateStore(
              ctx->builder.CreateLoad((expVal->isImplicitPointer()
                                           ? expVal->getType()->getLLVMType()
                                           : expVal->getType()
                                                 ->asReference()
                                                 ->getSubType()
                                                 ->getLLVMType()),
                                      expVal->getLLVM()),
              lhsVal->getLLVM());
        } else {
          SHOW("Normal assignment store")
          ctx->builder.CreateStore(expVal->getLLVM(), lhsVal->getLLVM());
        }
        return nullptr;
      } else {
        ctx->Error("Type of the left hand side of the assignment is " +
                       ctx->highlightError(lhsType->toString()) +
                       " and the type of right hand side is " +
                       ctx->highlightError(expType->toString()) +
                       ". The types of both sides of the assignment are not "
                       "compatible. Please check the logic.",
                   fileRange);
      }
    } else {
      ctx->Error("Left hand side of the assignment cannot be assigned to",
                 fileRange);
    }
  } else {
    if (lhsVal->getType()->isReference()) {
      ctx->Error("Left hand side of the assignment cannot be assigned to "
                 "because the referred type of the reference does not have "
                 "variability",
                 fileRange);
    } else if (lhsVal->getType()->isPointer()) {
      ctx->Error("Left hand side of the assignment cannot be assigned to "
                 "because it is of pointer type. If you intend to change the "
                 "value pointed to by this pointer, consider dereferencing it "
                 "before assigning",
                 fileRange);
    } else {
      ctx->Error(
          "Left hand side of the assignment cannot be assigned to because "
          "it is not a variable value",
          fileRange);
    }
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
