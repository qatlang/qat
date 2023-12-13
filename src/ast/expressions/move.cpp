#include "./move.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/maybe.hpp"

namespace qat::ast {

Move::Move(Expression* _exp, FileRange _fileRange) : Expression(std::move(_fileRange)), exp(_exp) {}

IR::Value* Move::emit(IR::Context* ctx) {
  auto* expEmit = exp->emit(ctx);
  auto* expTy   = expEmit->getType();
  if (expEmit->isImplicitPointer() || expTy->isReference()) {
    if (expTy->isReference()) {
      expEmit->loadImplicitPointer(ctx->builder);
    }
    auto* candTy = expEmit->isReference() ? expEmit->getType()->asReference()->getSubType() : expEmit->getType();
    if (!isAssignment) {
      if (candTy->isMoveConstructible()) {
        const bool didNotHaveCreateIn = !canCreateIn();
        if (isLocalDecl()) {
          if (!candTy->isSame(localValue->getType())) {
            ctx->Error("The type provided in the local declaration does not match the type of the value to be moved",
                       fileRange);
          }
          createIn = new IR::Value(localValue->getAlloca(), IR::ReferenceType::get(true, candTy, ctx), false,
                                   IR::Nature::temporary);
        } else if (!canCreateIn()) {
          createIn =
              new IR::Value(IR::Logic::newAlloca(ctx->getActiveFunction(), utils::unique_id(), candTy->getLLVMType()),
                            candTy, isVar, IR::Nature::temporary);
        }
        (void)candTy->moveConstructValue(ctx, createIn, expEmit, ctx->getActiveFunction());
        if (expEmit->isLocalToFn()) {
          ctx->getActiveFunction()->getBlock()->addMovedValue(expEmit->getLocalID());
        }
        if (didNotHaveCreateIn) {
          return new IR::Value(ctx->builder.CreateLoad(candTy->getLLVMType(), createIn->getLLVM()), candTy, false,
                               IR::Nature::temporary);
        } else {
          return createIn;
        }
      } else if (candTy->isTriviallyMovable()) {
        if (isLocalDecl()) {
          if (!candTy->isSame(localValue->getType())) {
            ctx->Error("The type provided in the local declaration does not match the type of the value to be moved",
                       fileRange);
          }
          createIn = new IR::Value(localValue->getAlloca(), IR::ReferenceType::get(true, candTy, ctx), false,
                                   IR::Nature::temporary);
        }
        if (canCreateIn()) {
          ctx->builder.CreateStore(ctx->builder.CreateLoad(candTy->getLLVMType(), expEmit->getLLVM()),
                                   createIn->getLLVM());
          ctx->builder.CreateStore(llvm::Constant::getNullValue(candTy->getLLVMType()), expEmit->getLLVM());
          if (expEmit->isLocalToFn()) {
            ctx->getActiveFunction()->getBlock()->addMovedValue(expEmit->getLocalID());
          }
          return createIn;
        } else {
          auto* loadRes = ctx->builder.CreateLoad(candTy->getLLVMType(), expEmit->getLLVM());
          ctx->builder.CreateStore(llvm::Constant::getNullValue(candTy->getLLVMType()), expEmit->getLLVM());
          return new IR::Value(loadRes, candTy, false, IR::Nature::temporary);
        }
      } else {
        ctx->Error((candTy->isCoreType() ? "Core type " : (candTy->isMix() ? "Mix type " : "Type ")) +
                       ctx->highlightError(candTy->toString()) +
                       " does not have a move constructor and is also not trivially movable",
                   fileRange);
      }
    } else {
      if (candTy->isTriviallyMovable()) {
        ctx->builder.CreateStore(ctx->builder.CreateLoad(candTy->getLLVMType(), expEmit->getLLVM()),
                                 createIn->getLLVM());
        ctx->builder.CreateStore(llvm::Constant::getNullValue(candTy->getLLVMType()), expEmit->getLLVM());
        if (expEmit->isLocalToFn()) {
          ctx->getActiveFunction()->getBlock()->addMovedValue(expEmit->getLocalID());
        }
        return createIn;
      } else if (candTy->isMoveAssignable()) {
        (void)candTy->moveAssignValue(ctx, createIn, expEmit, ctx->getActiveFunction());
        if (expEmit->isLocalToFn()) {
          ctx->getActiveFunction()->getBlock()->addMovedValue(expEmit->getLocalID());
        }
        return createIn;
      } else {
        ctx->Error((candTy->isCoreType() ? "Core type " : (candTy->isMix() ? "Mix type " : "Type ")) +
                       ctx->highlightError(candTy->toString()) +
                       " does not have a move assignment operator and is also not trivially movable",
                   fileRange);
      }
    }
  } else {
    ctx->Error(
        "This expression is a value. Expression provided should either be a reference, a local or a global variable. The provided value does not reside in an address and hence cannot be moved",
        fileRange);
  }
  return nullptr;
}

Json Move::toJson() const {
  return Json()._("nodeType", "move")._("expression", exp->toJson())._("fileRange", fileRange);
}

} // namespace qat::ast