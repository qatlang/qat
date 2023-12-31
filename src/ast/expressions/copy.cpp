#include "./copy.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/qat_type.hpp"

namespace qat::ast {

Copy::Copy(Expression* _exp, bool _isExpSelf, FileRange _fileRange)
    : Expression(std::move(_fileRange)), exp(_exp), isExpSelf(_isExpSelf) {}

IR::Value* Copy::emit(IR::Context* ctx) {
  if (isExpSelf) {
    if (!ctx->getActiveFunction()->isMemberFunction()) {
      ctx->Error("Cannot perform copy on the parent instance as this is not a member function", fileRange);
    } else {
      auto memFn = (IR::MemberFunction*)ctx->getActiveFunction();
      if (memFn->isStaticFunction()) {
        ctx->Error("Cannot perform copy on the parent instance as this is a static function", fileRange);
      }
      if (memFn->isConstructor()) {
        ctx->Error("Cannot perform copy on the parent instance as this is a constructor", fileRange);
      }
    }
  } else {
    if (exp->nodeType() == NodeType::self) {
      ctx->Error("Do not use this syntax for copying the parent instance. Use " + ctx->highlightError("''copy") +
                     " instead",
                 fileRange);
    }
  }
  auto* expEmit = isExpSelf ? ctx->getActiveFunction()->getFirstBlock()->getValue("''") : exp->emit(ctx);
  auto* expTy   = expEmit->getType();
  if (expEmit->isImplicitPointer() || expTy->isReference()) {
    if (expTy->isReference()) {
      expEmit->loadImplicitPointer(ctx->builder);
    }
    auto* candTy = expEmit->isReference() ? expEmit->getType()->asReference()->getSubType() : expEmit->getType();
    if (!isAssignment) {
      if (candTy->isCopyConstructible()) {
        bool shouldLoadValue = false;
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
          if (irName.has_value()) {
            createIn = ctx->getActiveFunction()->getBlock()->newValue(irName->value, candTy, isVar, irName->range);
          } else {
            shouldLoadValue = true;
            createIn        = new IR::Value(IR::Logic::newAlloca(ctx->getActiveFunction(), None, candTy->getLLVMType()),
                                            IR::ReferenceType::get(true, candTy, ctx), false, IR::Nature::temporary);
          }
        }
        (void)candTy->copyConstructValue(ctx, createIn, expEmit, ctx->getActiveFunction());
        if (shouldLoadValue) {
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
      if (createIn->isImplicitPointer()
              ? createIn->getType()->isSame(candTy)
              : (createIn->isReference() && createIn->getType()->asReference()->getSubType()->isSame(candTy))) {
        if (expEmit->isReference()) {
          expEmit->loadImplicitPointer(ctx->builder);
        }
        if (candTy->isTriviallyCopyable()) {
          ctx->builder.CreateStore(ctx->builder.CreateLoad(candTy->getLLVMType(), expEmit->getLLVM()),
                                   createIn->getLLVM());
          return createIn;
        } else if (candTy->isCopyAssignable()) {
          (void)candTy->copyAssignValue(ctx, createIn, expEmit, ctx->getActiveFunction());
          return createIn;
        } else {
          ctx->Error((candTy->isCoreType() ? "Core type " : (candTy->isMix() ? "Mix type " : "Type ")) +
                         ctx->highlightError(candTy->toString()) +
                         " does not have a copy assignment operator and is also not trivially copyable",
                     fileRange);
        }
      } else {
        ctx->Error("Tried to optimise copy assignment by in-place copying. The left hand side is of type " +
                       ctx->highlightError(createIn->getType()->toString()) + " but the right hand side is of type " +
                       ctx->highlightError(expEmit->getType()->toString()) +
                       " and hence copy assignment cannot be performed",
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