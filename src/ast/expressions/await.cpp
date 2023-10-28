#include "./await.hpp"
#include "../../IR/control_flow.hpp"
#include "../../IR/types/future.hpp"
#include "../../IR/types/void.hpp"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"

namespace qat::ast {

Await::Await(Expression* _exp, FileRange _range) : Expression(std::move(_range)), exp(_exp) {}

IR::Value* Await::emit(IR::Context* ctx) {
  SHOW("Exp nodetype is: " << (int)exp->nodeType())
  SHOW("Starting await emitting")
  if (!ctx->getActiveFunction()->isAsyncFunction()) {
    if (ctx->getActiveFunction()->getFullName() != "main") {
      ctx->Error("Please make the function async. " + ctx->highlightError("await") +
                     " can be used only inside an async function, expect in the case of the main function",
                 fileRange);
    }
  }
  auto* expEmit = exp->emit(ctx);
  if ((expEmit->isReference() && expEmit->getType()->asReference()->getSubType()->isFuture()) ||
      (expEmit->isImplicitPointer() && expEmit->getType()->isFuture())) {
    auto* futureTy = expEmit->isReference() ? expEmit->getType()->asReference()->getSubType()->asFuture()
                                            : expEmit->getType()->asFuture();
    if (expEmit->isReference()) {
      expEmit->loadImplicitPointer(ctx->builder);
    }
    auto* fun        = ctx->getActiveFunction();
    auto* trueBlock  = new IR::Block(fun, fun->getBlock());
    auto* falseBlock = new IR::Block(fun, fun->getBlock());
    auto* restBlock  = new IR::Block(fun, nullptr);
    restBlock->linkPrevBlock(fun->getBlock());
    ctx->builder.CreateCondBr(
        ctx->builder.CreateICmpEQ(
            ctx->builder.CreateLoad(
                llvm::Type::getInt1Ty(ctx->llctx),
                ctx->builder.CreateLoad(llvm::Type::getInt1Ty(ctx->llctx)->getPointerTo(),
                                        ctx->builder.CreateStructGEP(futureTy->getLLVMType(), expEmit->getLLVM(), 2),
                                        true)),
            llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u)),
        trueBlock->getBB(), falseBlock->getBB());
    trueBlock->setActive(ctx->builder);
    (void)IR::addBranch(ctx->builder, restBlock->getBB());
    falseBlock->setActive(ctx->builder);
    ctx->builder.CreateCondBr(
        ctx->builder.CreateICmpEQ(
            ctx->builder.CreateLoad(
                llvm::Type::getInt1Ty(ctx->llctx),
                ctx->builder.CreateLoad(llvm::Type::getInt1Ty(ctx->llctx)->getPointerTo(),
                                        ctx->builder.CreateStructGEP(futureTy->getLLVMType(), expEmit->getLLVM(), 2),
                                        true)),
            llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u)),
        restBlock->getBB(), falseBlock->getBB());
    (void)IR::addBranch(ctx->builder, restBlock->getBB());
    restBlock->setActive(ctx->builder);
    if (futureTy->getSubType()->isVoid()) {
      return new IR::Value(nullptr, IR::VoidType::get(ctx->llctx), false, IR::Nature::temporary);
    } else {
      return new IR::Value(
          ctx->builder.CreateLoad(futureTy->getSubType()->getLLVMType()->getPointerTo(),
                                  ctx->builder.CreateStructGEP(futureTy->getLLVMType(), expEmit->getLLVM(), 3)),
          IR::ReferenceType::get(expEmit->isReference() ? expEmit->getType()->asReference()->isSubtypeVariable()
                                                        : expEmit->isVariable(),
                                 futureTy->getSubType(), ctx),
          false, IR::Nature::temporary);
    }
  } else {
    ctx->Error("The expression should be a " + ctx->highlightError("future") + " to use await", exp->fileRange);
  }
  return nullptr;
}

Json Await::toJson() const {
  return Json()._("nodeType", "await")._("expression", exp->toJson())._("fileRange", fileRange);
}

} // namespace qat::ast