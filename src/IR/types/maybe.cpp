#include "./maybe.hpp"
#include "../../memory_tracker.hpp"
#include "../context.hpp"
#include "../control_flow.hpp"
#include "../value.hpp"
#include "./array.hpp"
#include "reference.hpp"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"

namespace qat::IR {

MaybeType::MaybeType(QatType* _subType, IR::Context* ctx) : subTy(_subType) {
  // TODO - Error/warn if subtype is opaque
  if (hasSizedSubType(ctx)) {
    llvmType = llvm::StructType::create(ctx->llctx, {llvm::Type::getInt1Ty(ctx->llctx), subTy->getLLVMType()},
                                        "maybe:[" + subTy->toString() + "]", false);
  } else {
    llvmType = llvm::Type::getInt1Ty(ctx->llctx);
  }
}

MaybeType* MaybeType::get(QatType* subTy, IR::Context* ctx) {
  for (auto* typ : types) {
    if (typ->isMaybe()) {
      if (typ->asMaybe()->getSubType()->isSame(subTy)) {
        return typ->asMaybe();
      }
    }
  }
  return new MaybeType(subTy, ctx);
}

bool MaybeType::isTypeSized() const { return true; }

bool MaybeType::hasSizedSubType(IR::Context* ctx) const { return subTy->isTypeSized(); }

QatType* MaybeType::getSubType() const { return subTy; }

String MaybeType::toString() const { return "maybe " + subTy->toString(); }

bool MaybeType::isDestructible() const { return subTy->isDestructible(); }

void MaybeType::destroyValue(IR::Context* ctx, Vec<IR::Value*> vals, IR::Function* fun) {
  if (isDestructible() && !vals.empty()) {
    auto* mInst = vals.front();
    if (mInst->isReference()) {
      mInst->loadImplicitPointer(ctx->builder);
    }
    auto* inst      = mInst->getLLVM();
    auto* currBlock = fun->getBlock();
    auto* trueBlock = new IR::Block(fun, currBlock);
    auto* restBlock = new IR::Block(fun, currBlock->getParent());
    restBlock->linkPrevBlock(currBlock);
    ctx->builder.CreateCondBr(
        ctx->builder.CreateLoad(llvm::Type::getInt1Ty(ctx->llctx), ctx->builder.CreateStructGEP(llvmType, inst, 0u)),
        trueBlock->getBB(), restBlock->getBB());
    trueBlock->setActive(ctx->builder);
    subTy->destroyValue(ctx,
                        {new IR::Value(ctx->builder.CreateStructGEP(llvmType, inst, 1u),
                                       IR::ReferenceType::get(false, subTy, ctx), false, IR::Nature::temporary)},
                        fun);
    (void)IR::addBranch(ctx->builder, restBlock->getBB());
    restBlock->setActive(ctx->builder);
  }
}

} // namespace qat::IR