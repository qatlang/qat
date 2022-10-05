#include "./assignment.hpp"
#include "../../IR/logic.hpp"
#include "../constants/integer_literal.hpp"
#include "../constants/null_pointer.hpp"
#include "../constants/unsigned_literal.hpp"
#include "../expressions/copy.hpp"
#include "../expressions/default.hpp"
#include "../expressions/move.hpp"
#include <cerrno>

namespace qat::ast {

#define MAX_RESPONSIVE_BITWIDTH 64u

Assignment::Assignment(Expression* _lhs, Expression* _value, utils::FileRange _fileRange)
    : Sentence(std::move(_fileRange)), lhs(_lhs), value(_value) {}

IR::Value* Assignment::emit(IR::Context* ctx) {
  auto* lhsVal = lhs->emit(ctx);
  if (value->nodeType() == NodeType::nullPointer) {
    auto* nullVal = (NullPointer*)value;
    if (lhsVal->getType()->isPointer()) {
      nullVal->setType(lhsVal->getType()->asPointer()->isSubtypeVariable(),
                       lhsVal->getType()->asPointer()->getSubType());
    } else if (lhsVal->getType()->isReference() && lhsVal->getType()->asReference()->getSubType()->isPointer()) {
      nullVal->setType(lhsVal->getType()->asReference()->getSubType()->asPointer()->isSubtypeVariable(),
                       lhsVal->getType()->asReference()->getSubType()->asPointer()->getSubType());
    } else {
      ctx->Error("Type of the LHS is not compatible with the RHS, which is a "
                 "null pointer",
                 value->fileRange);
    }
  } else if (value->nodeType() == NodeType::Default) {
    auto* defVal = (Default*)value;
    if (lhsVal->getType()->isReference()) {
      defVal->setType(lhsVal->getType()->asReference()->getSubType());
    } else {
      defVal->setType(lhsVal->getType());
    }
  } else if (value->nodeType() == NodeType::unsignedLiteral) {
    auto* uLit = (UnsignedLiteral*)value;
    if (lhsVal->getType()->isReference() && lhsVal->getType()->asReference()->getSubType()->isInteger()) {
      uLit->setType(lhsVal->getType()->asReference()->getSubType());
    } else if (lhsVal->getType()->isReference() &&
               lhsVal->getType()->asReference()->getSubType()->isUnsignedInteger()) {
      uLit->setType(lhsVal->getType()->asReference()->getSubType());
    } else if (lhsVal->getType()->isInteger()) {
      uLit->setType(lhsVal->getType());
    } else if (lhsVal->getType()->isUnsignedInteger()) {
      uLit->setType(lhsVal->getType());
    } else {
      ctx->Error("Type of LHS is not compatible with the RHS", fileRange);
    }
  } else if (value->nodeType() == NodeType::integerLiteral) {
    auto* iLit = (IntegerLiteral*)value;
    if (lhsVal->getType()->isReference() && lhsVal->getType()->asReference()->getSubType()->isInteger()) {
      iLit->setType(lhsVal->getType()->asReference()->getSubType());
    } else if (lhsVal->getType()->isReference() &&
               lhsVal->getType()->asReference()->getSubType()->isUnsignedInteger()) {
      iLit->setType(lhsVal->getType()->asReference()->getSubType());
    } else if (lhsVal->getType()->isInteger()) {
      iLit->setType(lhsVal->getType());
    } else if (lhsVal->getType()->isUnsignedInteger()) {
      iLit->setType(lhsVal->getType());
    } else {
      ctx->Error("Type of LHS is not compatible with the RHS", fileRange);
    }
  }
  SHOW("Emitted lhs and rhs of Assignment")
  if (lhsVal->isVariable() ||
      (lhsVal->getType()->isReference() && lhsVal->getType()->asReference()->isSubtypeVariable())) {
    SHOW("Is variable nature")
    if (lhsVal->isImplicitPointer() || lhsVal->getType()->isReference()) {
      if (value->nodeType() == NodeType::copyExpression || value->nodeType() == NodeType::moveExpression) {
        auto isCopy = value->nodeType() == NodeType::copyExpression;
        if ((lhsVal->getType()->isCoreType() && (isCopy ? lhsVal->getType()->asCore()->hasCopyAssignment()
                                                        : lhsVal->getType()->asCore()->hasMoveAssignment())) ||
            (lhsVal->getType()->isReference() && lhsVal->getType()->asReference()->getSubType()->isCoreType() &&
             ((isCopy ? lhsVal->getType()->asReference()->getSubType()->asCore()->hasCopyAssignment()
                      : lhsVal->getType()->asReference()->getSubType()->asCore()->hasMoveAssignment())))) {
          auto* val    = isCopy ? ((Copy*)value)->exp : ((Move*)value)->exp;
          auto* expVal = val->emit(ctx);
          auto* lhsTy  = lhsVal->getType();
          auto* expTy  = expVal->getType();
          if ((expTy->isCoreType() && expVal->isImplicitPointer()) ||
              (expTy->isReference() && expTy->asReference()->getSubType()->isCoreType())) {
            if (expTy->isReference()) {
              expVal->loadImplicitPointer(ctx->builder);
            }
            if (lhsTy->isSame(expVal->getType()) ||
                (lhsTy->isReference() && lhsTy->asReference()->getSubType()->isSame(
                                             expTy->isReference() ? expTy->asReference()->getSubType() : expTy)) ||
                (expTy->isReference() && expTy->asReference()->getSubType()->isSame(
                                             lhsTy->isReference() ? lhsTy->asReference()->getSubType() : lhsTy))) {
              auto* cTy = lhsTy->isCoreType() ? lhsTy->asCore() : lhsTy->asReference()->getSubType()->asCore();
              if (lhsVal->isImplicitPointer() && lhsVal->isReference()) {
                SHOW("LHS is implicit pointer")
                lhsVal->loadImplicitPointer(ctx->builder);
                SHOW("Loaded implicit pointer")
              }
              if (expVal->isImplicitPointer() && expTy->isReference()) {
                expVal->loadImplicitPointer(ctx->builder);
              }
              auto* cFn = isCopy ? cTy->getCopyAssignment() : cTy->getMoveAssignment();
              SHOW("Performing explicit " << (isCopy ? "copy" : "move") << " for core type " << cTy->getFullName()
                                          << " in assignment")
              (void)cFn->call(ctx, {lhsVal->getLLVM(), expVal->getLLVM()}, ctx->getMod());
              return nullptr;
            } else {
              ctx->Error("Types of the LHS and RHS are not compatible for assignment", fileRange);
            }
          } else {
            ctx->Error(String("The expression cannot be ") + (isCopy ? "copied" : "moved"), value->fileRange);
          }
        }
      }
      auto* expVal = value->emit(ctx);
      SHOW("Getting IR types")
      auto* lhsTy = lhsVal->getType();
      auto* expTy = expVal->getType();
      if (lhsTy->isSame(expTy) ||
          (lhsTy->isReference() && lhsTy->asReference()->getSubType()->isSame(
                                       expTy->isReference() ? expTy->asReference()->getSubType() : expTy)) ||
          (expTy->isReference() && expTy->asReference()->getSubType()->isSame(
                                       lhsTy->isReference() ? lhsTy->asReference()->getSubType() : lhsTy))) {
        SHOW("The general types are the same")
        if (lhsVal->isImplicitPointer() && lhsTy->isReference()) {
          SHOW("LHS is implicit pointer")
          lhsVal->loadImplicitPointer(ctx->builder);
          SHOW("Loaded implicit pointer")
        }
        if (expTy->isReference() || expVal->isImplicitPointer()) {
          if (expTy->isReference()) {
            expVal->loadImplicitPointer(ctx->builder);
            SHOW("Expression for assignment is of type " << expTy->asReference()->getSubType()->toString())
            expTy = expTy->asReference()->getSubType();
          }
          if (expTy->isCoreType() && (expTy->asCore()->hasCopyAssignment() || expTy->asCore()->hasMoveAssignment())) {
            auto* cTy = expTy->asCore();
            if (cTy->hasCopyAssignment()) {
              auto* cpFn = cTy->getCopyAssignment();
              ctx->Warning("Copy assignment of core type " + ctx->highlightWarning(cTy->getFullName()) +
                               " is invoked here. Please make the copy explicit",
                           fileRange);
              (void)cpFn->call(ctx, {lhsVal->getLLVM(), expVal->getLLVM()}, ctx->getMod());
            } else {
              auto* mvFn = cTy->getMoveAssignment();
              ctx->Warning("Move assignment of core type " + ctx->highlightWarning(cTy->getFullName()) +
                               " is invoked here. Please make the move explicit",
                           fileRange);
              (void)mvFn->call(ctx, {lhsVal->getLLVM(), expVal->getLLVM()}, ctx->getMod());
            }
            return nullptr;
          } else {
            expVal = new IR::Value(ctx->builder.CreateLoad(expTy->getLLVMType(), expVal->getLLVM()), expVal->getType(),
                                   expVal->isVariable(), expVal->getNature());
          }
        }
        SHOW("Creating store")
        ctx->builder.CreateStore(expVal->getLLVM(), lhsVal->getLLVM());
        return nullptr;
      } else if (expVal->isConstVal() &&
                 ((lhsTy->isInteger() || (lhsTy->isReference() && lhsTy->asReference()->getSubType()->isInteger()) ||
                   lhsTy->isUnsignedInteger() ||
                   (lhsTy->isReference() && lhsTy->asReference()->getSubType()->isUnsignedInteger())) &&
                  ((expTy->isInteger() && (expTy->asInteger()->getBitwidth() < MAX_RESPONSIVE_BITWIDTH)) ||
                   (expTy->isUnsignedInteger() &&
                    (expTy->asUnsignedInteger()->getBitwidth() <= MAX_RESPONSIVE_BITWIDTH))))) {
        if (lhsVal->isImplicitPointer() && lhsTy->isReference()) {
          SHOW("LHS is implicit pointer")
          lhsVal->loadImplicitPointer(ctx->builder);
        }
        auto isSignedLHS =
            lhsTy->isInteger() || (lhsTy->isReference() && lhsTy->asReference()->getSubType()->isInteger());
        auto isSignedExp = expTy->isInteger();
        auto lhsBits     = lhsTy->isInteger()
                               ? lhsTy->asInteger()->getBitwidth()
                               : (lhsTy->isUnsignedInteger()
                                      ? lhsTy->asUnsignedInteger()->getBitwidth()
                                      : (lhsTy->asReference()->getSubType()->isInteger()
                                             ? lhsTy->asReference()->getSubType()->asInteger()->getBitwidth()
                                             : lhsTy->asReference()->getSubType()->asUnsignedInteger()->getBitwidth()));
        auto expBits =
            expTy->isInteger() ? expTy->asInteger()->getBitwidth() : expTy->asUnsignedInteger()->getBitwidth();
        if (isSignedExp == isSignedLHS) {
          if (lhsBits < expBits) {
            ctx->Warning("The biwidth of the RHS is higher than LHS and hence, there could be a loss of data",
                         fileRange);
          }
          expVal = new IR::Value(
              ctx->builder.CreateIntCast(expVal->asConst()->getLLVM(),
                                         lhsTy->isReference() ? lhsTy->asReference()->getSubType()->getLLVMType()
                                                              : lhsTy->getLLVMType(),
                                         isSignedLHS),
              lhsTy->isReference() ? lhsTy->asReference()->getSubType() : lhsTy, false, IR::Nature::pure);
          ctx->builder.CreateStore(expVal->getLLVM(), lhsVal->getLLVM());
        } else {
          ctx->Error("Types on both sides are not compatible", fileRange);
        }
      } else {
        ctx->Error("Type of the left hand side of the assignment is " + ctx->highlightError(lhsTy->toString()) +
                       " and the type of right hand side is " + ctx->highlightError(expTy->toString()) +
                       ". The types of both sides of the assignment are not "
                       "compatible. Please check the logic.",
                   fileRange);
      }
    } else {
      ctx->Error("Left hand side of the assignment cannot be assigned to", lhs->fileRange);
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
      ctx->Error("Left hand side of the assignment cannot be assigned to because "
                 "it is not a variable value",
                 lhs->fileRange);
    }
  }
}

Json Assignment::toJson() const {
  return Json()._("nodeType", "assignment")._("lhs", lhs->toJson())._("rhs", value->toJson())._("fileRange", fileRange);
}

} // namespace qat::ast
