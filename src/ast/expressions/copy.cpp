#include "./copy.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/qat_type.hpp"

namespace qat::ast {

Copy::Copy(Expression* _exp, FileRange _fileRange) : Expression(std::move(_fileRange)), exp(_exp) {}

IR::Value* Copy::emit(IR::Context* ctx) {
  auto* expEmit = exp->emit(ctx);
  auto* expTy   = expEmit->getType();
  if (expEmit->isImplicitPointer() || expTy->isReference()) {
    if (expTy->isReference()) {
      expEmit->loadImplicitPointer(ctx->builder);
    }
    auto* candTy = expEmit->isReference() ? expEmit->getType()->asReference()->getSubType() : expEmit->getType();
    if (!isAssignment) {
      if (candTy->isCopyConstructible()) {
        const bool didNotHaveCreateIn = !canCreateIn();
        if (isLocalDecl()) {
          if (!candTy->isSame(localValue->getType())) {
            ctx->Error("The type provided in the local declaration does not match the type of the value to be copied",
                       fileRange);
          }
          createIn = new IR::Value(localValue->getAlloca(), IR::ReferenceType::get(true, candTy, ctx), false,
                                   IR::Nature::temporary);
        } else if (canCreateIn()) {
          if (createIn->isReference() || createIn->isImplicitPointer()) {
            auto expTy =
                createIn->isImplicitPointer() ? createIn->getType() : createIn->getType()->asReference()->getSubType();
            if (!expTy->isSame(candTy)) {
              ctx->Error("Trying to optimise the copy by creating in-place, but the expression type is " +
                             ctx->highlightError(candTy->toString()) +
                             " which does not match with the underlying type for in-place creation which is " +
                             ctx->highlightError(expTy->toString()),
                         fileRange);
            }
          } else {
            ctx->Error("Trying to optimise the copy by creating in-place, but the containing type is not a reference",
                       fileRange);
          }
        } else {
          createIn =
              new IR::Value(IR::Logic::newAlloca(ctx->getActiveFunction(), utils::unique_id(), candTy->getLLVMType()),
                            candTy, isVar, IR::Nature::temporary);
        }
        (void)candTy->copyConstructValue(ctx, createIn, expEmit, ctx->getActiveFunction());
        if (didNotHaveCreateIn) {
          return new IR::Value(ctx->builder.CreateLoad(candTy->getLLVMType(), createIn->getLLVM()), candTy, false,
                               IR::Nature::temporary);
        } else {
          return createIn;
        }
      } else if (candTy->isTriviallyCopyable()) {
        if (isLocalDecl()) {
          if (!candTy->isSame(localValue->getType())) {
            ctx->Error("The type provided in the local declaration does not match the type of the value to be copied",
                       fileRange);
          }
          createIn = new IR::Value(localValue->getAlloca(), IR::ReferenceType::get(true, candTy, ctx), false,
                                   IR::Nature::temporary);
        }
        if (canCreateIn()) {
          if (!isLocalDecl()) {
            if (createIn->isReference() || createIn->isImplicitPointer()) {
              auto expTy = createIn->isImplicitPointer() ? createIn->getType()
                                                         : createIn->getType()->asReference()->getSubType();
              if (!expTy->isSame(candTy)) {
                ctx->Error("Trying to optimise the copy by creating in-place, but the expression type is " +
                               ctx->highlightError(candTy->toString()) +
                               " which does not match with the underlying type for in-place creation which is " +
                               ctx->highlightError(expTy->toString()),
                           fileRange);
              }
            } else {
              ctx->Error("Trying to optimise the copy by creating in-place, but the containing type is not a reference",
                         fileRange);
            }
          }
          // FIXME - Use memcpy?
          ctx->builder.CreateStore(ctx->builder.CreateLoad(candTy->getLLVMType(), expEmit->getLLVM()),
                                   createIn->getLLVM());
          return createIn;
        } else {
          return new IR::Value(ctx->builder.CreateLoad(candTy->getLLVMType(), expEmit->getLLVM()), candTy, false,
                               IR::Nature::temporary);
        }
      } else {
        ctx->Error((candTy->isCoreType() ? "Core type " : (candTy->isMix() ? "Mix type " : "Type ")) +
                       ctx->highlightError(candTy->toString()) +
                       " does not have a copy constructor and is also not trivially copyable",
                   fileRange);
      }
    } else {
      if (candTy->isCopyAssignable()) {
        (void)candTy->copyAssignValue(ctx, createIn, expEmit, ctx->getActiveFunction());
        return createIn;
      } else if (candTy->isTriviallyCopyable()) {
        ctx->builder.CreateStore(ctx->builder.CreateLoad(candTy->getLLVMType(), expEmit->getLLVM()),
                                 createIn->getLLVM());
        return createIn;
      } else {
        ctx->Error((candTy->isCoreType() ? "Core type " : (candTy->isMix() ? "Mix type " : "Type ")) +
                       ctx->highlightError(candTy->toString()) +
                       " does not have a copy assignment operator and is also not trivially copyable",
                   fileRange);
      }
    }
  } else {
    ctx->Error(
        "This expression is a value. Expression provided should either be a reference, a local or a global variable. The provided value does not reside in an address and hence cannot be copied from",
        fileRange);
  }
  return nullptr;
}

Json Copy::toJson() const {
  return Json()._("nodeType", "copyExpression")._("expression", exp->toJson())._("fileRange", fileRange);
}

} // namespace qat::ast