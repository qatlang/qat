#include "./default.hpp"

namespace qat::ast {

Default::Default(utils::FileRange _fileRange)
    : Expression(std::move(_fileRange)) {}

void Default::setType(IR::QatType *typ) { candidateType = typ; }

IR::Value *Default::emit(IR::Context *ctx) {
  if (candidateType) {
    if (candidateType->isInteger()) {
      ctx->Warning("The recognised type for the default expression is a signed "
                   "integer. The resultant value is 0. But using the literal "
                   "is recommended over using default for readability and "
                   "decreased clutter",
                   fileRange);
      if (irName.has_value()) {
        auto *block = ctx->fn->getBlock();
        auto *loc   = block->newValue(irName.value(), candidateType, isVar);
        ctx->builder.CreateStore(
            llvm::ConstantInt::get(candidateType->getLLVMType(), 0u),
            loc->getAlloca());
        return loc;
      } else {
        return new IR::Value(llvm::ConstantInt::get(
                                 candidateType->asInteger()->getLLVMType(), 0u),
                             candidateType, false, IR::Nature::pure);
      }
    } else if (candidateType->isUnsignedInteger()) {
      ctx->Warning(
          "The recognised type for the default expression is an unsigned "
          "integer. The resultant value is 0. But using the literal "
          "is recommended over using default for readability and "
          "decreased clutter",
          fileRange);
      if (irName.has_value()) {
        auto *block = ctx->fn->getBlock();
        auto *loc   = block->newValue(irName.value(), candidateType, isVar);
        ctx->builder.CreateStore(
            llvm::ConstantInt::get(candidateType->getLLVMType(), 0u),
            loc->getAlloca());
        return loc;
      } else {
        return new IR::Value(
            llvm::ConstantInt::get(
                candidateType->asUnsignedInteger()->getLLVMType(), 0u),
            candidateType, false, IR::Nature::pure);
      }
    } else if (candidateType->isPointer()) {
      ctx->Error(
          "The recognised type for the default expression is a pointer. The "
          "obvious value here is a null pointer, however it is not allowed to "
          "use default as an alternative to null, for improved readability",
          fileRange);
    } else if (candidateType->isReference()) {
      ctx->Error("Cannot get default value for a reference type", fileRange);
    } else if (candidateType->isCoreType()) {
      auto *cTy = candidateType->asCore();
      if (cTy->hasDefaultConstructor()) {
        auto *defFn = cTy->getDefaultConstructor();
        auto *block = ctx->fn->getBlock();
        if (irName.has_value()) {
          auto *loc = block->newValue(irName.value(), candidateType, isVar);
          (void)defFn->call(ctx, {loc->getAlloca()}, ctx->getMod());
          return loc;
        } else {
          auto *loc = block->newValue(utils::unique_id(), cTy, true);
          (void)defFn->call(ctx, {loc->getAlloca()}, ctx->getMod());
          auto *res = new IR::Value(loc->getAlloca(), cTy, false,
                                    IR::Nature::temporary);
          res->setLocalID(loc->getLocalID());
          return res;
        }
      } else {
        ctx->Error("Core type " + ctx->highlightError(cTy->getFullName()) +
                       " does not have a default constructor. Please check "
                       "logic and make necessary changes",
                   fileRange);
      }
    } else {
      ctx->Error("The type " + ctx->highlightError(candidateType->toString()) +
                     " cannot have default value",
                 fileRange);
      // FIXME - Handle other types
    }
  } else {
    ctx->Error("No type available to get the default value of", fileRange);
  }
}

Json Default::toJson() const { return Json()._("nodeType", "default"); }

} // namespace qat::ast