#include "./assignment.hpp"
#include "../expressions/default.hpp"
#include "../expressions/null_pointer.hpp"

namespace qat::ast {

Assignment::Assignment(Expression *_lhs, Expression *_value,
                       utils::FileRange _fileRange)
    : Sentence(std::move(_fileRange)), lhs(_lhs), value(_value) {}

IR::Value *Assignment::emit(IR::Context *ctx) {
  auto *lhsVal = lhs->emit(ctx);
  if (value->nodeType() == NodeType::nullPointer) {
    auto *nullVal = (NullPointer *)value;
    if (lhsVal->getType()->isPointer()) {
      nullVal->setType(lhsVal->getType()->asPointer()->isSubtypeVariable(),
                       lhsVal->getType()->asPointer()->getSubType());
    } else if (lhsVal->getType()->isReference() &&
               lhsVal->getType()->asReference()->getSubType()->isPointer()) {
      nullVal->setType(lhsVal->getType()
                           ->asReference()
                           ->getSubType()
                           ->asPointer()
                           ->isSubtypeVariable(),
                       lhsVal->getType()
                           ->asReference()
                           ->getSubType()
                           ->asPointer()
                           ->getSubType());
    } else {
      ctx->Error("Type of the LHS is not compatible with the RHS, which is a "
                 "null pointer",
                 value->fileRange);
    }
  } else if (value->nodeType() == NodeType::Default) {
    auto *defVal = (Default *)value;
    if (lhsVal->getType()->isReference()) {
      defVal->setType(lhsVal->getType()->asReference()->getSubType());
    } else {
      defVal->setType(lhsVal->getType());
    }
  }
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
           lhsType->asReference()->getSubType()->isSame(
               expType->isReference() ? expType->asReference()->getSubType()
                                      : expType)) ||
          (expType->isReference() &&
           expType->asReference()->getSubType()->isSame(
               lhsType->isReference() ? lhsType->asReference()->getSubType()
                                      : expType))) {
        SHOW("The general types are the same")
        if (lhsVal->isImplicitPointer() && lhsType->isReference()) {
          SHOW("LHS is implicit pointer")
          lhsVal->loadImplicitPointer(ctx->builder);
        }
        SHOW("Loaded implicit pointer")
        if (expType->isReference() || expVal->isImplicitPointer()) {
          if (expType->isReference()) {
            SHOW("Expression for assignment is of type "
                 << expType->asReference()->getSubType()->toString())
            expType = expType->asReference()->getSubType();
          }
          expVal = new IR::Value(ctx->builder.CreateLoad(expType->getLLVMType(),
                                                         expVal->getLLVM()),
                                 expVal->getType(), expVal->isVariable(),
                                 expVal->getNature());
        }
        SHOW("Creating store")
        ctx->builder.CreateStore(expVal->getLLVM(), lhsVal->getLLVM());
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
                 lhs->fileRange);
    }
  } else {
    if (lhsVal->getType()->isReference()) {
      ctx->Error("Left hand side of the assignment cannot be assigned to "
                 "because the reference does not have variability",
                 lhs->fileRange);
    } else if (lhsVal->getType()->isPointer()) {
      ctx->Error("Left hand side of the assignment cannot be assigned to "
                 "because it is of pointer type. If you intend to change the "
                 "value pointed to by this pointer, consider dereferencing it "
                 "before assigning",
                 lhs->fileRange);
    } else {
      ctx->Error(
          "Left hand side of the assignment cannot be assigned to because "
          "it is not a variable value",
          lhs->fileRange);
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
