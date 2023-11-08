#include "./mix_type_initialiser.hpp"
#include "../../IR/types/maybe.hpp"
#include "../../utils/split_string.hpp"
#include "../constants/integer_literal.hpp"
#include "../constants/unsigned_literal.hpp"
#include "array_literal.hpp"
#include "default.hpp"
#include "none.hpp"

namespace qat::ast {

MixTypeInitialiser::MixTypeInitialiser(QatType* _type, String _subName, Maybe<Expression*> _expression,
                                       FileRange _fileRange)
    : Expression(std::move(_fileRange)), type(_type), subName(std::move(_subName)), expression(_expression) {}

IR::Value* MixTypeInitialiser::emit(IR::Context* ctx) {
  // FIXME - Support heaped value
  auto  reqInfo  = ctx->getAccessInfo();
  auto* typeEmit = type->emit(ctx);
  if (typeEmit->isMix()) {
    auto* mixTy = typeEmit->asMix();
    if (mixTy->isAccessible(ctx->getAccessInfo())) {
      auto subRes = mixTy->hasSubTypeWithName(subName);
      if (subRes.first) {
        if (subRes.second) {
          if (!expression.has_value()) {
            ctx->Error("Field " + ctx->highlightError(subName) + " of mix type " +
                           ctx->highlightError(mixTy->getFullName()) + " expects a value to be associated with it",
                       fileRange);
          }
        } else {
          if (expression.has_value()) {
            ctx->Error("Field " + ctx->highlightError(subName) + " of mix type " +
                           ctx->highlightError(mixTy->getFullName()) + " cannot have any value associated with it",
                       fileRange);
          }
        }
        llvm::Value* exp = nullptr;
        if (subRes.second) {
          auto* typ = mixTy->getSubTypeWithName(subName);
          if (expression.value()->hasTypeInferrance()) {
            expression.value()->asTypeInferrable()->setInferenceType(typ);
          }
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
          exp = llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, mixTy->getDataBitwidth()), 0u, true);
        }
        auto* index = llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, mixTy->getTagBitwidth()),
                                             mixTy->getIndexOfName(subName));
        if (isLocalDecl()) {
          if (localValue->getType()->isSame(mixTy)) {
            createIn = localValue->toNewIRValue();
          } else {
            ctx->Error("The type of the local declaration is not compatible here", fileRange);
          }
        } else if (!createIn) {
          createIn = ctx->getActiveFunction()->getBlock()->newValue(
              irName.has_value() ? irName->value : utils::unique_id(), mixTy, isVar,
              irName.has_value() ? irName->range : fileRange);
        }
        SHOW("Creating mix store")
        ctx->builder.CreateStore(index, ctx->builder.CreateStructGEP(mixTy->getLLVMType(), createIn->getLLVM(), 0));
        if (subRes.second) {
          ctx->builder.CreateStore(exp, ctx->builder.CreatePointerCast(
                                            ctx->builder.CreateStructGEP(mixTy->getLLVMType(), createIn->getLLVM(), 1),
                                            mixTy->getSubTypeWithName(subName)->getLLVMType()->getPointerTo()));
        }
        return createIn;
      } else {
        ctx->Error("No field named " + ctx->highlightError(subName) + " is present inside mix type " +
                       ctx->highlightError(mixTy->getFullName()),
                   fileRange);
      }
    } else {
      ctx->Error("Mix type " + ctx->highlightError(mixTy->getFullName()) + " is not accessible here", fileRange);
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