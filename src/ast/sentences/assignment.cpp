#include "./assignment.hpp"
#include "../../IR/control_flow.hpp"
#include "../../IR/function.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/maybe.hpp"
#include "../constants/integer_literal.hpp"
#include "../constants/null_pointer.hpp"
#include "../constants/unsigned_literal.hpp"
#include "../expressions/copy.hpp"
#include "../expressions/default.hpp"
#include "../expressions/move.hpp"
#include "../expressions/none.hpp"
#include "llvm/IR/Constants.h"
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
    } else if (lhsVal->getType()->isMaybe() && lhsVal->getType()->asMaybe()->getSubType()->isPointer()) {
      nullVal->setType(lhsVal->getType()->asMaybe()->getSubType()->asPointer()->isSubtypeVariable(),
                       lhsVal->getType()->asMaybe()->getSubType());
    } else if (lhsVal->getType()->isReference() && lhsVal->getType()->asReference()->getSubType()->isMaybe() &&
               lhsVal->getType()->asReference()->getSubType()->asMaybe()->getSubType()->isPointer()) {
      nullVal->setType(
          lhsVal->getType()->asReference()->getSubType()->asMaybe()->getSubType()->asPointer()->isSubtypeVariable(),
          lhsVal->getType()->asReference()->getSubType()->asMaybe()->getSubType());
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
    } else if (lhsVal->getType()->isMaybe() && (lhsVal->getType()->asMaybe()->getSubType()->isInteger() ||
                                                lhsVal->getType()->asMaybe()->getSubType()->isUnsignedInteger())) {
      uLit->setType(lhsVal->getType()->asMaybe()->getSubType());
    } else if (lhsVal->getType()->isReference() && lhsVal->getType()->asReference()->getSubType()->isMaybe() &&
               (lhsVal->getType()->asReference()->getSubType()->asMaybe()->getSubType()->isInteger() ||
                lhsVal->getType()->asReference()->getSubType()->asMaybe()->getSubType()->isUnsignedInteger())) {
      uLit->setType(lhsVal->getType()->asReference()->getSubType()->asMaybe()->getSubType());
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
    } else if (lhsVal->getType()->isMaybe() && (lhsVal->getType()->asMaybe()->getSubType()->isInteger() ||
                                                lhsVal->getType()->asMaybe()->getSubType()->isUnsignedInteger())) {
      iLit->setType(lhsVal->getType()->asMaybe()->getSubType());
    } else if (lhsVal->getType()->isReference() && lhsVal->getType()->asReference()->getSubType()->isMaybe() &&
               (lhsVal->getType()->asReference()->getSubType()->asMaybe()->getSubType()->isInteger() ||
                lhsVal->getType()->asReference()->getSubType()->asMaybe()->getSubType()->isUnsignedInteger())) {
      iLit->setType(lhsVal->getType()->asReference()->getSubType()->asMaybe()->getSubType());
    } else {
      ctx->Error("Type of LHS is not compatible with the RHS", fileRange);
    }
  }
  SHOW("Emitted lhs and rhs of Assignment")
  if (lhsVal->getType()->isMaybe() ||
      (lhsVal->getType()->isReference() && lhsVal->getType()->asReference()->getSubType()->isMaybe())) {
    IR::MaybeType* mTy = nullptr;
    if (lhsVal->getType()->isMaybe()) {
      mTy = lhsVal->getType()->asMaybe();
      if (lhsVal->isImplicitPointer()) {
        if (!lhsVal->isVariable()) {
          ctx->Error("Left hand side does not have variability and hence cannot be assigned to", fileRange);
        }
      } else {
        ctx->Error(
            "The expression is a value and hence cannot be reassigned. Assignment requires an address on the left hand side",
            fileRange);
      }
    } else {
      mTy = lhsVal->getType()->asReference()->getSubType()->asMaybe();
      if (lhsVal->getType()->asReference()->isSubtypeVariable()) {
        lhsVal->loadImplicitPointer(ctx->builder);
      } else {
        ctx->Error("The left hand side is a reference without variability and hence cannot be assigned to", fileRange);
      }
    }
    llvm::Value* maybeTagLhsRef   = ctx->builder.CreateStructGEP(mTy->getLLVMType(), lhsVal->getLLVM(), 0u);
    llvm::Value* maybeValueLhsRef = ctx->builder.CreateStructGEP(mTy->getLLVMType(), lhsVal->getLLVM(), 1u);
    auto*        subTy            = mTy->getSubType();
    if (value->nodeType() == NodeType::none || value->nodeType() == NodeType::Default) {
      if (value->nodeType() == NodeType::none) {
        auto* noneAST = ((ast::NoneExpression*)value);
        if (noneAST->type) {
          auto* noneTy = noneAST->type->emit(ctx);
          if (!subTy->isSame(noneTy)) {
            ctx->Error("The type of the none expression is " + ctx->highlightError(noneTy->toString()) +
                           " but the left hand side is maybe type, with " + ctx->highlightError(subTy->toString()) +
                           " as the sub type",
                       value->fileRange);
          }
        }
      }
      if (subTy->isCoreType() && subTy->asCore()->hasDestructor()) {
        auto* tagTrueBlock = new IR::Block(ctx->fn, ctx->fn->getBlock());
        auto* restBlock    = new IR::Block(ctx->fn, ctx->fn->getBlock()->getParent());
        restBlock->linkPrevBlock(ctx->fn->getBlock());
        ctx->builder.CreateCondBr(
            ctx->builder.CreateICmpEQ(ctx->builder.CreateLoad(llvm::Type::getInt1Ty(ctx->llctx), maybeTagLhsRef),
                                      llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u)),
            tagTrueBlock->getBB(), restBlock->getBB());
        tagTrueBlock->setActive(ctx->builder);
        auto* dstrFn = subTy->asCore()->getDestructor();
        (void)dstrFn->call(ctx, {maybeValueLhsRef}, ctx->getMod());
        ctx->builder.CreateBr(restBlock->getBB());
        restBlock->setActive(ctx->builder);
      }
      ctx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 0u), maybeTagLhsRef);
      // FIXME - Storing null for the value is necessary only if there is no destructor
      ctx->builder.CreateStore(llvm::Constant::getNullValue(subTy->getLLVMType()), maybeValueLhsRef);
      return nullptr;
    } else {
      auto* exp = value->emit(ctx);
      if ((exp->getType()->isMaybe() && mTy->getSubType()->isSame(exp->getType()->asMaybe()->getSubType()) &&
           exp->isImplicitPointer()) ||
          (exp->getType()->isReference() && exp->getType()->asReference()->getSubType()->isMaybe() &&
           mTy->getSubType()->isSame(exp->getType()->asReference()->getSubType()->asMaybe()->getSubType()))) {
        // NOTE - Both are references
        if (subTy->isCoreType()) {
          auto* cTy         = subTy->asCore();
          auto* falseBool   = llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 0u);
          auto* trueBool    = llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u);
          auto* maybeTagLhs = ctx->builder.CreateLoad(llvm::Type::getInt1Ty(ctx->llctx), maybeTagLhsRef);
          auto* maybeTagRhs = ctx->builder.CreateLoad(
              llvm::Type::getInt1Ty(ctx->llctx), ctx->builder.CreateStructGEP(mTy->getLLVMType(), exp->getLLVM(), 0u));
          auto* maybeValueRhsRef = ctx->builder.CreateStructGEP(mTy->getLLVMType(), exp->getLLVM(), 1u);
          auto* currBlock        = ctx->fn->getBlock();
          auto* condBothTrue     = ctx->builder.CreateAnd(
              {ctx->builder.CreateICmpEQ(maybeTagLhs, trueBool), ctx->builder.CreateICmpEQ(maybeTagRhs, trueBool)});
          auto* firstTrueBlock  = new IR::Block(ctx->fn, currBlock);
          auto* firstFalseBlock = new IR::Block(ctx->fn, currBlock);
          ctx->builder.CreateCondBr(condBothTrue, firstTrueBlock->getBB(), firstFalseBlock->getBB());
          firstTrueBlock->setActive(ctx->builder);
          if (cTy->hasCopyAssignment() || cTy->hasMoveAssignment()) {
            IR::Function* candFn = nullptr;
            if (cTy->hasCopyAssignment()) {
              candFn = cTy->getCopyAssignment();
            } else {
              candFn = cTy->getMoveAssignment();
            }
            (void)candFn->call(ctx, {maybeValueLhsRef, maybeValueRhsRef}, ctx->getMod());
          } else {
            ctx->builder.CreateStore(ctx->builder.CreateLoad(cTy->getLLVMType(), maybeValueRhsRef), maybeValueLhsRef);
          }
          (void)IR::addBranch(ctx->builder, firstFalseBlock->getBB());
          firstFalseBlock->setActive(ctx->builder);
          auto* condTrueAndFalse = ctx->builder.CreateAnd(
              {ctx->builder.CreateICmpEQ(maybeTagLhs, trueBool), ctx->builder.CreateICmpEQ(maybeTagRhs, falseBool)});
          auto* secTrueBlock  = new IR::Block(ctx->fn, currBlock);
          auto* secFalseBlock = new IR::Block(ctx->fn, currBlock);
          ctx->builder.CreateCondBr(condTrueAndFalse, secTrueBlock->getBB(), secFalseBlock->getBB());
          secTrueBlock->setActive(ctx->builder);
          if (cTy->hasDestructor()) {
            (void)cTy->getDestructor()->call(ctx, {maybeTagLhsRef}, ctx->getMod());
          }
          ctx->builder.CreateStore(falseBool, maybeTagLhsRef);
          // FIXME - Storing null for the value is necessary only if there is no destructor
          ctx->builder.CreateStore(llvm::Constant::getNullValue(cTy->getLLVMType()), maybeValueLhsRef);
          (void)IR::addBranch(ctx->builder, secFalseBlock->getBB());
          secFalseBlock->setActive(ctx->builder);
          auto* condFalseAndTrue = ctx->builder.CreateAnd(
              {ctx->builder.CreateICmpEQ(maybeTagLhs, falseBool), ctx->builder.CreateICmpEQ(maybeTagRhs, trueBool)});
          auto* thirdTrueBlock = new IR::Block(ctx->fn, currBlock);
          auto* restBlock      = new IR::Block(ctx->fn, currBlock->getParent());
          restBlock->linkPrevBlock(currBlock);
          ctx->builder.CreateCondBr(condFalseAndTrue, thirdTrueBlock->getBB(), restBlock->getBB());
          thirdTrueBlock->setActive(ctx->builder);
          if (cTy->hasCopyConstructor() || cTy->hasMoveConstructor()) {
            IR::Function* candFn = nullptr;
            if (cTy->hasCopyConstructor()) {
              candFn = cTy->getCopyConstructor();
            } else {
              candFn = cTy->getMoveConstructor();
            }
            (void)candFn->call(ctx, {maybeValueLhsRef, maybeValueRhsRef}, ctx->getMod());
          } else {
            ctx->builder.CreateStore(ctx->builder.CreateLoad(cTy->getLLVMType(), maybeValueRhsRef), maybeValueLhsRef);
          }
          ctx->builder.CreateStore(trueBool, maybeTagLhsRef);
          (void)IR::addBranch(ctx->builder, restBlock->getBB());
          restBlock->setActive(ctx->builder);
        } else {
          if (exp->isReference()) {
            exp->loadImplicitPointer(ctx->builder);
            exp = new IR::Value(
                ctx->builder.CreateLoad(exp->getType()->asReference()->getSubType()->getLLVMType(), exp->getLLVM()),
                exp->getType()->asReference()->getSubType(), exp->getType()->asReference()->isSubtypeVariable(),
                IR::Nature::temporary);
          } else {
            exp->loadImplicitPointer(ctx->builder);
          }
          ctx->builder.CreateStore(exp->getLLVM(), lhsVal->getLLVM());
        }
        return nullptr;
      } else if (exp->getType()->isMaybe() && exp->getType()->asMaybe()->getSubType()->isSame(mTy->getSubType())) {
        // NOTE - Assigning absolute value to the reference
        if (subTy->isCoreType() && subTy->asCore()->hasDestructor()) {
          auto* condLhsTrue =
              ctx->builder.CreateICmpEQ(ctx->builder.CreateLoad(llvm::Type::getInt1Ty(ctx->llctx), maybeTagLhsRef),
                                        llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u));
          auto* trueBlock = new IR::Block(ctx->fn, ctx->fn->getBlock());
          auto* restBlock = new IR::Block(ctx->fn, ctx->fn->getBlock()->getParent());
          restBlock->linkPrevBlock(ctx->fn->getBlock());
          ctx->builder.CreateCondBr(condLhsTrue, trueBlock->getBB(), restBlock->getBB());
          trueBlock->setActive(ctx->builder);
          (void)subTy->asCore()->getDestructor()->call(
              ctx, {ctx->builder.CreateStructGEP(mTy->getLLVMType(), lhsVal->getLLVM(), 1u)}, ctx->getMod());
          (void)IR::addBranch(ctx->builder, restBlock->getBB());
          restBlock->setActive(ctx->builder);
        }
        // FIXME - Automatically allocate the value and perform copy/move?
        ctx->builder.CreateStore(exp->getLLVM(), lhsVal->getLLVM());
      } else if (exp->getType()->isSame(mTy->getSubType()) ||
                 (exp->getType()->isReference() &&
                  exp->getType()->asReference()->getSubType()->isSame(mTy->getSubType()))) {
        if (exp->getType()->isReference()) {
          exp->loadImplicitPointer(ctx->builder);
        }
        if (subTy->isCoreType() &&
            ((exp->getType()->isSame(mTy->getSubType()) && exp->isImplicitPointer()) || exp->isReference()) &&
            (subTy->asCore()->hasCopy() || subTy->asCore()->hasMove() || subTy->asCore()->hasDestructor())) {
          auto* cTy = subTy->asCore();
          auto* condLhsTrue =
              ctx->builder.CreateICmpEQ(ctx->builder.CreateLoad(llvm::Type::getInt1Ty(ctx->llctx), maybeTagLhsRef),
                                        llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u));
          auto* trueBlock  = new IR::Block(ctx->fn, ctx->fn->getBlock());
          auto* falseBlock = new IR::Block(ctx->fn, ctx->fn->getBlock());
          auto* restBlock  = new IR::Block(ctx->fn, ctx->fn->getBlock());
          restBlock->linkPrevBlock(ctx->fn->getBlock());
          ctx->builder.CreateCondBr(condLhsTrue, trueBlock->getBB(), falseBlock->getBB());
          trueBlock->setActive(ctx->builder);
          if (cTy->hasCopyAssignment() || cTy->hasMoveAssignment()) {
            auto* candFn = cTy->hasCopyAssignment() ? cTy->getCopyAssignment() : cTy->getMoveAssignment();
            (void)candFn->call(ctx, {maybeValueLhsRef, exp->getLLVM()}, ctx->getMod());
          } else {
            if (cTy->hasDestructor()) {
              auto* dstrFn = cTy->getDestructor();
              (void)dstrFn->call(ctx, {maybeValueLhsRef}, ctx->getMod());
            }
            ctx->builder.CreateStore(ctx->builder.CreateLoad(cTy->getLLVMType(), exp->getLLVM()), maybeValueLhsRef);
          }
          (void)IR::addBranch(ctx->builder, restBlock->getBB());
          falseBlock->setActive(ctx->builder);
          if (cTy->hasCopyConstructor() || cTy->hasMoveConstructor()) {
            auto* constrFn = cTy->hasCopyConstructor() ? cTy->getCopyConstructor() : cTy->getMoveConstructor();
            (void)constrFn->call(ctx, {maybeValueLhsRef, exp->getLLVM()}, ctx->getMod());
          } else {
            ctx->builder.CreateStore(ctx->builder.CreateLoad(cTy->getLLVMType(), exp->getLLVM()), maybeValueLhsRef);
          }
          ctx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u), maybeTagLhsRef);
          (void)IR::addBranch(ctx->builder, restBlock->getBB());
          restBlock->setActive(ctx->builder);
        } else {
          if (exp->isReference()) {
            exp->loadImplicitPointer(ctx->builder);
            exp = new IR::Value(ctx->builder.CreateLoad(subTy->getLLVMType(), exp->getLLVM()), subTy, false,
                                IR::Nature::temporary);
          } else if (exp->isImplicitPointer()) {
            exp = new IR::Value(ctx->builder.CreateLoad(subTy->getLLVMType(), exp->getLLVM()), subTy, false,
                                IR::Nature::temporary);
          }
          ctx->builder.CreateStore(exp->getLLVM(), maybeValueLhsRef);
          ctx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u), maybeTagLhsRef);
        }
      } else {
        ctx->Error("Type of the right hand side of the assignment is not compatible with the left hand side",
                   fileRange);
      }
    }
  } else if (lhsVal->isVariable() ||
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
  return nullptr;
}

Json Assignment::toJson() const {
  return Json()._("nodeType", "assignment")._("lhs", lhs->toJson())._("rhs", value->toJson())._("fileRange", fileRange);
}

} // namespace qat::ast
