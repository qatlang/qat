#include "./maybe.hpp"
#include "../../memory_tracker.hpp"
#include "../context.hpp"
#include "../control_flow.hpp"
#include "../value.hpp"
#include "./array.hpp"
#include "reference.hpp"
#include "llvm/Analysis/ConstantFolding.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"

namespace qat::IR {

MaybeType::MaybeType(QatType* _subType, bool _isPacked, IR::Context* ctx) : subTy(_subType), isPacked(_isPacked) {
  linkingName = "qat'maybe:[" + String(isPacked ? "pack," : "") + subTy->getNameForLinking() + "]";
  // TODO - Error/warn if subtype is opaque
  llvmType = llvm::StructType::create(ctx->llctx,
                                      {llvm::Type::getInt1Ty(ctx->llctx),
                                       hasSizedSubType(ctx) ? subTy->getLLVMType() : llvm::Type::getInt8Ty(ctx->llctx)},
                                      linkingName, false);
}

MaybeType* MaybeType::get(QatType* subTy, bool isPacked, IR::Context* ctx) {
  for (auto* typ : allQatTypes) {
    if (typ->isMaybe()) {
      if (typ->asMaybe()->getSubType()->isSame(subTy)) {
        return typ->asMaybe();
      }
    }
  }
  return new MaybeType(subTy, isPacked, ctx);
}

bool MaybeType::isTypeSized() const { return true; }

bool MaybeType::isTypePacked() const { return isPacked; }

bool MaybeType::hasSizedSubType(IR::Context* ctx) const { return subTy->isTypeSized(); }

QatType* MaybeType::getSubType() const { return subTy; }

String MaybeType::toString() const { return "maybe:[" + String(isPacked ? "pack, " : "") + subTy->toString() + "]"; }

bool MaybeType::isCopyConstructible() const { return isTriviallyCopyable() || subTy->isCopyConstructible(); }

void MaybeType::copyConstructValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) {
  if (isTriviallyCopyable()) {
    ctx->builder.CreateStore(ctx->builder.CreateLoad(getLLVMType(), second->getLLVM()), first->getLLVM());
  } else {
    if (isCopyConstructible()) {
      auto* tag        = ctx->builder.CreateLoad(llvm::Type::getInt1Ty(ctx->llctx),
                                                 ctx->builder.CreateStructGEP(getLLVMType(), second->getLLVM(), 0u));
      auto* trueBlock  = new IR::Block(fun, fun->getBlock());
      auto* falseBlock = new IR::Block(fun, fun->getBlock());
      auto* restBlock  = new IR::Block(fun, fun->getBlock()->getParent());
      restBlock->linkPrevBlock(fun->getBlock());
      ctx->builder.CreateCondBr(
          ctx->builder.CreateICmpEQ(tag, llvm::ConstantInt::getTrue(llvm::Type::getInt1Ty(ctx->llctx))),
          trueBlock->getBB(), falseBlock->getBB());
      trueBlock->setActive(ctx->builder);
      subTy->copyConstructValue(ctx,
                                new IR::Value(ctx->builder.CreateStructGEP(getLLVMType(), first->getLLVM(), 1u),
                                              IR::ReferenceType::get(true, subTy, ctx), false, IR::Nature::temporary),
                                new IR::Value(ctx->builder.CreateStructGEP(getLLVMType(), second->getLLVM(), 1u),
                                              IR::ReferenceType::get(false, subTy, ctx), false, IR::Nature::temporary),
                                fun);
      ctx->builder.CreateStore(llvm::ConstantInt::getTrue(llvm::Type::getInt1Ty(ctx->llctx)),
                               ctx->builder.CreateStructGEP(getLLVMType(), first->getLLVM(), 0u));
      (void)IR::addBranch(ctx->builder, restBlock->getBB());
      falseBlock->setActive(ctx->builder);
      ctx->builder.CreateStore(llvm::Constant::getNullValue(getLLVMType()), first->getLLVM());
      (void)IR::addBranch(ctx->builder, restBlock->getBB());
      restBlock->setActive(ctx->builder);
    } else {
      ctx->Error("Could not copy construct an instance of type " + ctx->highlightError(toString()), None);
    }
  }
}

bool MaybeType::isCopyAssignable() const { return isTriviallyCopyable() || subTy->isCopyAssignable(); }

void MaybeType::copyAssignValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) {
  if (isTriviallyCopyable()) {
    ctx->builder.CreateStore(ctx->builder.CreateLoad(getLLVMType(), second->getLLVM()), first->getLLVM());
  } else {
    if (isCopyAssignable()) {
      auto* firstTag           = ctx->builder.CreateLoad(llvm::Type::getInt1Ty(ctx->llctx),
                                                         ctx->builder.CreateStructGEP(getLLVMType(), first->getLLVM(), 0u));
      auto* secondTag          = ctx->builder.CreateLoad(llvm::Type::getInt1Ty(ctx->llctx),
                                                         ctx->builder.CreateStructGEP(getLLVMType(), second->getLLVM(), 0u));
      auto* tagMatchTrueBlock  = new IR::Block(fun, fun->getBlock());
      auto* tagMatchFalseBlock = new IR::Block(fun, fun->getBlock());
      auto* restBlock          = new IR::Block(fun, fun->getBlock()->getParent());
      restBlock->linkPrevBlock(fun->getBlock());
      ctx->builder.CreateCondBr(ctx->builder.CreateICmpEQ(firstTag, secondTag), tagMatchTrueBlock->getBB(),
                                tagMatchFalseBlock->getBB());
      tagMatchTrueBlock->setActive(ctx->builder);
      auto* tagTrueBlock  = new IR::Block(fun, tagMatchTrueBlock);
      auto* tagFalseBlock = new IR::Block(fun, tagMatchTrueBlock);
      ctx->builder.CreateCondBr(
          ctx->builder.CreateICmpEQ(firstTag, llvm::ConstantInt::getTrue(llvm::Type::getInt1Ty(ctx->llctx))),
          tagTrueBlock->getBB(), tagFalseBlock->getBB());
      tagTrueBlock->setActive(ctx->builder);
      subTy->copyAssignValue(ctx,
                             new IR::Value(ctx->builder.CreateStructGEP(getLLVMType(), first->getLLVM(), 1u),
                                           IR::ReferenceType::get(true, subTy, ctx), false, IR::Nature::temporary),
                             new IR::Value(ctx->builder.CreateStructGEP(getLLVMType(), second->getLLVM(), 1u),
                                           IR::ReferenceType::get(false, subTy, ctx), false, IR::Nature::temporary),
                             fun);
      (void)IR::addBranch(ctx->builder, restBlock->getBB());
      tagFalseBlock->setActive(ctx->builder);
      (void)IR::addBranch(ctx->builder, restBlock->getBB());
      tagMatchFalseBlock->setActive(ctx->builder);
      auto* firstValueBlock  = new IR::Block(fun, tagMatchFalseBlock);
      auto* secondValueBlock = new IR::Block(fun, tagMatchFalseBlock);
      ctx->builder.CreateCondBr(
          ctx->builder.CreateICmpEQ(firstTag, llvm::ConstantInt::getTrue(llvm::Type::getInt1Ty(ctx->llctx))),
          firstValueBlock->getBB(), secondValueBlock->getBB());
      firstValueBlock->setActive(ctx->builder);
      subTy->destroyValue(ctx,
                          new IR::Value(ctx->builder.CreateStructGEP(getLLVMType(), first->getLLVM(), 1u),
                                        IR::ReferenceType::get(true, subTy, ctx), false, IR::Nature::temporary),
                          fun);
      ctx->builder.CreateStore(llvm::ConstantInt::getFalse(llvm::Type::getInt1Ty(ctx->llctx)),
                               ctx->builder.CreateStructGEP(getLLVMType(), first->getLLVM(), 0u));
      (void)IR::addBranch(ctx->builder, restBlock->getBB());
      secondValueBlock->setActive(ctx->builder);
      subTy->copyConstructValue(ctx,
                                new IR::Value(ctx->builder.CreateStructGEP(getLLVMType(), first->getLLVM(), 1u),
                                              IR::ReferenceType::get(true, subTy, ctx), false, IR::Nature::temporary),
                                new IR::Value(ctx->builder.CreateStructGEP(getLLVMType(), second->getLLVM(), 1u),
                                              IR::ReferenceType::get(false, subTy, ctx), false, IR::Nature::temporary),
                                fun);
      ctx->builder.CreateStore(llvm::ConstantInt::getTrue(llvm::Type::getInt1Ty(ctx->llctx)),
                               ctx->builder.CreateStructGEP(getLLVMType(), first->getLLVM(), 0u));
      (void)IR::addBranch(ctx->builder, restBlock->getBB());
      restBlock->setActive(ctx->builder);
    } else {
      ctx->Error("Could not copy assign an instance of type " + ctx->highlightError(toString()), None);
    }
  }
}

bool MaybeType::isMoveConstructible() const { return isTriviallyMovable() || subTy->isMoveConstructible(); }

void MaybeType::moveConstructValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) {
  if (isTriviallyCopyable()) {
    ctx->builder.CreateStore(ctx->builder.CreateLoad(getLLVMType(), second->getLLVM()), first->getLLVM());
    ctx->builder.CreateStore(llvm::Constant::getNullValue(getLLVMType()), second->getLLVM());
  } else {
    if (isMoveConstructible()) {
      auto* tag        = ctx->builder.CreateLoad(llvm::Type::getInt1Ty(ctx->llctx),
                                                 ctx->builder.CreateStructGEP(getLLVMType(), second->getLLVM(), 0u));
      auto* trueBlock  = new IR::Block(fun, fun->getBlock());
      auto* falseBlock = new IR::Block(fun, fun->getBlock());
      auto* restBlock  = new IR::Block(fun, fun->getBlock()->getParent());
      restBlock->linkPrevBlock(fun->getBlock());
      ctx->builder.CreateCondBr(
          ctx->builder.CreateICmpEQ(tag, llvm::ConstantInt::getTrue(llvm::Type::getInt1Ty(ctx->llctx))),
          trueBlock->getBB(), falseBlock->getBB());
      trueBlock->setActive(ctx->builder);
      subTy->moveConstructValue(ctx,
                                new IR::Value(ctx->builder.CreateStructGEP(getLLVMType(), first->getLLVM(), 1u),
                                              IR::ReferenceType::get(true, subTy, ctx), false, IR::Nature::temporary),
                                new IR::Value(ctx->builder.CreateStructGEP(getLLVMType(), second->getLLVM(), 1u),
                                              IR::ReferenceType::get(false, subTy, ctx), false, IR::Nature::temporary),
                                fun);
      ctx->builder.CreateStore(llvm::ConstantInt::getTrue(llvm::Type::getInt1Ty(ctx->llctx)),
                               ctx->builder.CreateStructGEP(getLLVMType(), first->getLLVM(), 0u));
      ctx->builder.CreateStore(llvm::ConstantInt::getFalse(llvm::Type::getInt1Ty(ctx->llctx)),
                               ctx->builder.CreateStructGEP(getLLVMType(), second->getLLVM(), 0u));
      (void)IR::addBranch(ctx->builder, restBlock->getBB());
      falseBlock->setActive(ctx->builder);
      ctx->builder.CreateStore(llvm::Constant::getNullValue(getLLVMType()), first->getLLVM());
      (void)IR::addBranch(ctx->builder, restBlock->getBB());
      restBlock->setActive(ctx->builder);
    } else {
      ctx->Error("Could not move construct an instance of type " + ctx->highlightError(toString()), None);
    }
  }
}

bool MaybeType::isMoveAssignable() const { return isTriviallyMovable() || subTy->isMoveAssignable(); }

void MaybeType::moveAssignValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) {
  if (isTriviallyMovable()) {
    ctx->builder.CreateStore(ctx->builder.CreateLoad(getLLVMType(), second->getLLVM()), first->getLLVM());
    ctx->builder.CreateStore(llvm::Constant::getNullValue(getLLVMType()), second->getLLVM());
  } else {
    if (isMoveAssignable()) {
      auto* firstTag           = ctx->builder.CreateLoad(llvm::Type::getInt1Ty(ctx->llctx),
                                                         ctx->builder.CreateStructGEP(getLLVMType(), first->getLLVM(), 0u));
      auto* secondTag          = ctx->builder.CreateLoad(llvm::Type::getInt1Ty(ctx->llctx),
                                                         ctx->builder.CreateStructGEP(getLLVMType(), second->getLLVM(), 0u));
      auto* tagMatchTrueBlock  = new IR::Block(fun, fun->getBlock());
      auto* tagMatchFalseBlock = new IR::Block(fun, fun->getBlock());
      auto* restBlock          = new IR::Block(fun, fun->getBlock()->getParent());
      restBlock->linkPrevBlock(fun->getBlock());
      ctx->builder.CreateCondBr(ctx->builder.CreateICmpEQ(firstTag, secondTag), tagMatchTrueBlock->getBB(),
                                tagMatchFalseBlock->getBB());
      tagMatchTrueBlock->setActive(ctx->builder);
      auto* tagTrueBlock  = new IR::Block(fun, tagMatchTrueBlock);
      auto* tagFalseBlock = new IR::Block(fun, tagMatchTrueBlock);
      ctx->builder.CreateCondBr(
          ctx->builder.CreateICmpEQ(firstTag, llvm::ConstantInt::getTrue(llvm::Type::getInt1Ty(ctx->llctx))),
          tagTrueBlock->getBB(), tagFalseBlock->getBB());
      tagTrueBlock->setActive(ctx->builder);
      subTy->moveAssignValue(ctx,
                             new IR::Value(ctx->builder.CreateStructGEP(getLLVMType(), first->getLLVM(), 1u),
                                           IR::ReferenceType::get(true, subTy, ctx), false, IR::Nature::temporary),
                             new IR::Value(ctx->builder.CreateStructGEP(getLLVMType(), second->getLLVM(), 1u),
                                           IR::ReferenceType::get(false, subTy, ctx), false, IR::Nature::temporary),
                             fun);
      ctx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 0u, false),
                               ctx->builder.CreateStructGEP(getLLVMType(), second->getLLVM(), 0u));
      (void)IR::addBranch(ctx->builder, restBlock->getBB());
      tagFalseBlock->setActive(ctx->builder);
      (void)IR::addBranch(ctx->builder, restBlock->getBB());
      tagMatchFalseBlock->setActive(ctx->builder);
      auto* firstValueBlock  = new IR::Block(fun, tagMatchFalseBlock);
      auto* secondValueBlock = new IR::Block(fun, tagMatchFalseBlock);
      ctx->builder.CreateCondBr(
          ctx->builder.CreateICmpEQ(firstTag, llvm::ConstantInt::getTrue(llvm::Type::getInt1Ty(ctx->llctx))),
          firstValueBlock->getBB(), secondValueBlock->getBB());
      firstValueBlock->setActive(ctx->builder);
      subTy->destroyValue(ctx,
                          new IR::Value(ctx->builder.CreateStructGEP(getLLVMType(), first->getLLVM(), 1u),
                                        IR::ReferenceType::get(true, subTy, ctx), false, IR::Nature::temporary),
                          fun);
      ctx->builder.CreateStore(llvm::ConstantInt::getFalse(llvm::Type::getInt1Ty(ctx->llctx)),
                               ctx->builder.CreateStructGEP(getLLVMType(), first->getLLVM(), 0u));
      (void)IR::addBranch(ctx->builder, restBlock->getBB());
      secondValueBlock->setActive(ctx->builder);
      subTy->moveConstructValue(ctx,
                                new IR::Value(ctx->builder.CreateStructGEP(getLLVMType(), first->getLLVM(), 1u),
                                              IR::ReferenceType::get(true, subTy, ctx), false, IR::Nature::temporary),
                                new IR::Value(ctx->builder.CreateStructGEP(getLLVMType(), second->getLLVM(), 1u),
                                              IR::ReferenceType::get(false, subTy, ctx), false, IR::Nature::temporary),
                                fun);
      ctx->builder.CreateStore(llvm::ConstantInt::getTrue(llvm::Type::getInt1Ty(ctx->llctx)),
                               ctx->builder.CreateStructGEP(getLLVMType(), first->getLLVM(), 0u));
      (void)IR::addBranch(ctx->builder, restBlock->getBB());
      restBlock->setActive(ctx->builder);
    } else {
      ctx->Error("Could not move assign an instance of type " + ctx->highlightError(toString()), None);
    }
  }
}

bool MaybeType::isDestructible() const { return subTy->isDestructible(); }

void MaybeType::destroyValue(IR::Context* ctx, IR::Value* instance, IR::Function* fun) {
  if (isDestructible()) {
    if (instance->isReference()) {
      instance->loadImplicitPointer(ctx->builder);
    }
    auto* inst      = instance->getLLVM();
    auto* currBlock = fun->getBlock();
    auto* trueBlock = new IR::Block(fun, currBlock);
    auto* restBlock = new IR::Block(fun, currBlock->getParent());
    restBlock->linkPrevBlock(currBlock);
    ctx->builder.CreateCondBr(
        ctx->builder.CreateLoad(llvm::Type::getInt1Ty(ctx->llctx), ctx->builder.CreateStructGEP(llvmType, inst, 0u)),
        trueBlock->getBB(), restBlock->getBB());
    trueBlock->setActive(ctx->builder);
    subTy->destroyValue(ctx,
                        new IR::Value(ctx->builder.CreateStructGEP(llvmType, inst, 1u),
                                      IR::ReferenceType::get(false, subTy, ctx), false, IR::Nature::temporary),
                        fun);
    (void)IR::addBranch(ctx->builder, restBlock->getBB());
    restBlock->setActive(ctx->builder);
  } else {
    ctx->Error("Could not destroy an instance of type " + ctx->highlightError(toString()), None);
  }
}

Maybe<String> MaybeType::toPrerunGenericString(IR::PrerunValue* value) const {
  if (canBePrerunGeneric()) {
    if (llvm::cast<llvm::ConstantInt>(value->getLLVMConstant()->getAggregateElement(0u))->getValue().getBoolValue()) {
      return "is(" +
             subTy->toPrerunGenericString(new IR::PrerunValue(value->getLLVMConstant()->getAggregateElement(0u), subTy))
                 .value() +
             ")";
    } else {
      return "none";
    }
  } else {
    return None;
  }
}

Maybe<bool> MaybeType::equalityOf(IR::Context* ctx, IR::PrerunValue* first, IR::PrerunValue* second) const {
  if (first->getType()->isSame(second->getType())) {
    const bool firstHasValue =
        llvm::cast<llvm::ConstantInt>(first->getLLVMConstant()->getAggregateElement(0u))->getValue().getBoolValue();
    const bool secondHasValue =
        llvm::cast<llvm::ConstantInt>(second->getLLVMConstant()->getAggregateElement(0u))->getValue().getBoolValue();
    if (firstHasValue == secondHasValue) {
      if (firstHasValue) {
        return subTy->equalityOf(
            ctx, new IR::PrerunValue(first->getLLVMConstant()->getAggregateElement(1u), subTy),
            new IR::PrerunValue(llvm::ConstantFoldConstant(
                                    llvm::ConstantExpr::getBitCast(second->getLLVMConstant()->getAggregateElement(1u),
                                                                   subTy->getLLVMType()),
                                    ctx->dataLayout.value()),
                                subTy));
      } else {
        return true;
      }
    } else {
      return false;
    }
  } else {
    return None;
  }
}

} // namespace qat::IR