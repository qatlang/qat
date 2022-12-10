#include "./mix_type_initialiser.hpp"
#include "../../IR/types/maybe.hpp"
#include "../../utils/split_string.hpp"

namespace qat::ast {

MixTypeInitialiser::MixTypeInitialiser(QatType* _type, String _subName, Maybe<Expression*> _expression,
                                       FileRange _fileRange)
    : Expression(std::move(_fileRange)), type(_type), subName(std::move(_subName)), expression(_expression) {}

IR::Value* MixTypeInitialiser::emit(IR::Context* ctx) {
  // FIXME - Support heaped value
  auto  reqInfo  = ctx->getReqInfo();
  auto* typeEmit = type->emit(ctx);
  if (typeEmit->isMix()) {
    auto* uTy = typeEmit->asMix();
    if (uTy->isAccessible(ctx->getReqInfo())) {
      auto subRes = uTy->hasSubTypeWithName(subName);
      if (subRes.first) {
        if (subRes.second) {
          if (!expression.has_value()) {
            ctx->Error("Field " + ctx->highlightError(subName) + " of mix type " +
                           ctx->highlightError(uTy->getFullName()) + " expects a value to be associated with it",
                       fileRange);
          }
        } else {
          if (expression.has_value()) {
            ctx->Error("Field " + ctx->highlightError(subName) + " of mix type " +
                           ctx->highlightError(uTy->getFullName()) + " cannot have any value associated with it",
                       fileRange);
          }
        }
        llvm::Value* exp = nullptr;
        if (subRes.second) {
          auto* typ     = uTy->getSubTypeWithName(subName);
          auto* expEmit = expression.value()->emit(ctx);
          if (typ->isSame(expEmit->getType())) {
            expEmit->loadImplicitPointer(ctx->builder);
            exp = expEmit->getLLVM();
          } else if (expEmit->isReference() && expEmit->getType()->asReference()->getSubType()->isSame(typ)) {
            exp = ctx->builder.CreateLoad(expEmit->getType()->asReference()->getSubType()->getLLVMType(),
                                          expEmit->getLLVM());
          } else if (typ->isReference() && typ->asReference()->getSubType()->isSame(expEmit->getType())) {
            if (expEmit->isImplicitPointer()) {
              exp = expEmit->getLLVM();
            } else {
              ctx->Error("The expected type is " + ctx->highlightError(typ->toString()) +
                             ", but the expression is of type " + ctx->highlightError(expEmit->getType()->toString()),
                         expression.value()->fileRange);
            }
          } else {
            ctx->Error("The expected type is " + ctx->highlightError(typ->toString()) +
                           ", but the expression is of type " + ctx->highlightError(expEmit->getType()->toString()),
                       expression.value()->fileRange);
          }
        } else {
          exp = llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, uTy->getDataBitwidth()), 0u, true);
        }
        auto*        index = llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, uTy->getTagBitwidth()),
                                                    uTy->getIndexOfName(subName));
        llvm::Value* newValue;
        if (local) {
          if (local->getType()->isMaybe() && local->getType()->asMaybe()->getSubType()->isSame(uTy)) {
            newValue = ctx->builder.CreateStructGEP(local->getType()->getLLVMType(), local->getAlloca(), 1u);
          } else if (local->getType()->isSame(uTy)) {
            newValue = local->getAlloca();
          } else {
            ctx->Error("The type of the local declaration is not compatible here", fileRange);
          }
        } else if (!irName.empty()) {
          local    = ctx->fn->getBlock()->newValue(irName, uTy, isVar);
          newValue = local->getAlloca();
        } else {
          newValue = ctx->builder.CreateAlloca(uTy->getLLVMType(), 0u);
        }
        SHOW("Creating mix store")
        ctx->builder.CreateStore(index, ctx->builder.CreateStructGEP(uTy->getLLVMType(), newValue, 0));
        if (subRes.second) {
          ctx->builder.CreateStore(
              exp, ctx->builder.CreatePointerCast(ctx->builder.CreateStructGEP(uTy->getLLVMType(), newValue, 1),
                                                  uTy->getSubTypeWithName(subName)->getLLVMType()->getPointerTo()));
        }
        if (local) {
          if (local->getType()->isMaybe()) {
            ctx->builder.CreateStore(
                llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u),
                ctx->builder.CreateStructGEP(local->getType()->getLLVMType(), local->getAlloca(), 0u));
          }
          auto* res = new IR::Value(local->getAlloca(), local->getType(), local->isVariable(), local->getNature());
          res->setLocalID(local->getLocalID());
          return res;
        } else {
          return new IR::Value(newValue, uTy, true, IR::Nature::pure);
        }
      } else {
        ctx->Error("No field named " + ctx->highlightError(subName) + " is present inside mix type " +
                       ctx->highlightError(uTy->getFullName()),
                   fileRange);
      }
    } else {
      ctx->Error("Mix type " + ctx->highlightError(uTy->getFullName()) + " is not accessible here", fileRange);
    }
  } else {
    ctx->Error(ctx->highlightError(typeEmit->toString()) + " is not a mix type", fileRange);
  }
  return nullptr;
}

Json MixTypeInitialiser::toJson() const {
  return Json()
      ._("nodeType", "mixTypeInitialiser")
      ._("type", type->toJson())
      ._("subName", subName)
      ._("hasExpression", expression.has_value())
      ._("expression", expression.has_value() ? expression.value()->toJson() : JsonValue())
      ._("fileRange", fileRange);
}

} // namespace qat::ast