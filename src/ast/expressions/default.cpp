#include "./default.hpp"
#include "../../IR/types/maybe.hpp"

namespace qat::ast {

Default::Default(Maybe<ast::QatType*> _providedType, FileRange _fileRange)
    : Expression(std::move(_fileRange)), providedType(_providedType) {}

IR::Value* Default::emit(IR::Context* ctx) {
  auto resultType = providedType.has_value() ? providedType.value()->emit(ctx) : inferredType.value_or(nullptr);
  if (resultType) {
    if (resultType->isInteger()) {
      ctx->Warning("The recognised type for the " + ctx->highlightWarning("default") +
                       " expression is a signed integer. The resultant value is 0. But using the literal "
                       "is recommended for readability and decreased clutter",
                   fileRange);
      if (irName.has_value()) {
        auto* block = ctx->fn->getBlock();
        auto* loc   = block->newValue(irName->value, resultType, isVar, irName->range);
        ctx->builder.CreateStore(llvm::ConstantInt::get(resultType->getLLVMType(), 0u), loc->getAlloca());
        return loc;
      } else {
        return new IR::Value(llvm::ConstantInt::get(resultType->asInteger()->getLLVMType(), 0u), resultType, false,
                             IR::Nature::pure);
      }
    } else if (resultType->isUnsignedInteger()) {
      ctx->Warning("The recognised type for the " + ctx->highlightWarning("default") +
                       " expression is an unsigned integer. The resultant value is 0. But using the literal "
                       "is recommended for readability and decreased clutter",
                   fileRange);
      if (irName.has_value()) {
        auto* block = ctx->fn->getBlock();
        auto* loc   = block->newValue(irName->value, resultType, isVar, irName->range);
        ctx->builder.CreateStore(llvm::ConstantInt::get(resultType->getLLVMType(), 0u), loc->getAlloca());
        return loc;
      } else {
        return new IR::Value(llvm::ConstantInt::get(resultType->asUnsignedInteger()->getLLVMType(), 0u), resultType,
                             false, IR::Nature::pure);
      }
    } else if (resultType->isPointer()) {
      ctx->Error("The recognised type for the default expression is a pointer. The "
                 "obvious value here is a null pointer, however it is not allowed to "
                 "use default as an alternative to null, for improved readability",
                 fileRange);
    } else if (resultType->isReference()) {
      ctx->Error("Cannot get default value for a reference type", fileRange);
    } else if (resultType->isCoreType()) {
      auto* cTy = resultType->asCore();
      if (cTy->hasDefaultConstructor()) {
        auto* defFn = cTy->getDefaultConstructor();
        if (!defFn->isAccessible(ctx->getAccessInfo())) {
          ctx->Error("The default constructor of " + ctx->highlightError(cTy->toString()) + " is not accessible here",
                     fileRange);
        }
        auto* block = ctx->fn->getBlock();
        if (irName.has_value()) {
          auto* loc = block->newValue(irName->value, resultType, isVar, irName->range);
          (void)defFn->call(ctx, {loc->getAlloca()}, ctx->getMod());
          return loc;
        } else {
          auto* loc = block->newValue(utils::unique_id(), cTy, true, fileRange);
          (void)defFn->call(ctx, {loc->getAlloca()}, ctx->getMod());
          auto* res = new IR::Value(loc->getAlloca(), cTy, false, IR::Nature::temporary);
          res->setLocalID(loc->getLocalID());
          return res;
        }
      } else {
        ctx->Error("Core type " + ctx->highlightError(cTy->getFullName()) +
                       " does not have a default constructor. Please check "
                       "logic and make necessary changes",
                   fileRange);
      }
    } else if (resultType->isMaybe()) {
      auto* mTy = resultType->asMaybe();
      ctx->Warning("The inferred type for " + ctx->highlightWarning("default") + " is " +
                       ctx->highlightWarning(mTy->toString()) + " and it's better to use " +
                       ctx->highlightWarning("none") + " for clarity",
                   fileRange);
      auto* block = ctx->fn->getBlock();
      auto* loc   = block->newValue(irName.has_value() ? irName->value : utils::unique_id(), mTy, true,
                                  irName.has_value() ? irName->range : fileRange);
      if (mTy->hasSizedSubType(ctx)) {
        ctx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 0u),
                                 ctx->builder.CreateStructGEP(mTy->getLLVMType(), loc->getLLVM(), 0u));
        ctx->builder.CreateStore(llvm::Constant::getNullValue(mTy->getSubType()->getLLVMType()),
                                 ctx->builder.CreateStructGEP(mTy->getLLVMType(), loc->getLLVM(), 1u));
      } else {
        ctx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 0u), loc->getLLVM());
      }
      auto* res = new IR::Value(loc->getAlloca(), mTy, false, IR::Nature::temporary);
      res->setLocalID(loc->getLocalID());
      return res;
    } else if (resultType->isChoice()) {
      auto* chTy = resultType->asChoice();
      if (chTy->hasDefault()) {
        chTy->getDefault();
      }
    } else {
      ctx->Error("The type " + ctx->highlightError(resultType->toString()) + " does not have default value", fileRange);
      // FIXME - Handle other types
    }
  } else {
    ctx->Error("default has no type inferred from scope, and there is no type provided like " +
                   ctx->highlightError("default:[someTy]"),
               fileRange);
  }
}

Json Default::toJson() const { return Json()._("nodeType", "default"); }

} // namespace qat::ast