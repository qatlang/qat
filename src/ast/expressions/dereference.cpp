#include "./dereference.hpp"

namespace qat::ast {

Dereference::Dereference(Expression* _exp, FileRange _fileRange) : Expression(std::move(_fileRange)), exp(_exp) {}

IR::Value* Dereference::emit(IR::Context* ctx) {
  auto* expEmit = exp->emit(ctx);
  auto* expTy   = expEmit->getType();
  if (expTy->isReference()) {
    expEmit->loadImplicitPointer(ctx->builder);
    expTy = expTy->asReference()->getSubType();
  }
  if (expTy->isPointer()) {
    if (expEmit->isReference()) {
      expEmit->loadImplicitPointer(ctx->builder);
      if (!expTy->asPointer()->isMulti()) {
        expEmit = new IR::Value(ctx->builder.CreateLoad(expTy->getLLVMType(), expEmit->getLLVM()), expTy, false,
                                IR::Nature::temporary);
      }
    } else if (expEmit->isImplicitPointer()) {
      if (!expTy->asPointer()->isMulti()) {
        expEmit->loadImplicitPointer(ctx->builder);
      }
    } else {
      if (expTy->asPointer()->isMulti()) {
        expEmit->makeImplicitPointer(ctx, None);
      }
    }
    return new IR::Value(
        expTy->asPointer()->isMulti()
            ? ctx->builder.CreateLoad(llvm::PointerType::get(expTy->asPointer()->getSubType()->getLLVMType(), 0u),
                                      ctx->builder.CreateStructGEP(expTy->getLLVMType(), expEmit->getLLVM(), 0u))
            : expEmit->getLLVM(),
        IR::ReferenceType::get(expTy->asPointer()->isSubtypeVariable(), expTy->asPointer()->getSubType(), ctx->llctx),
        false, IR::Nature::temporary);
  } else if (expTy->isCoreType()) {
    if (expTy->asCore()->hasUnaryOperator("#")) {
      if (!expEmit->isReference() && !expEmit->isImplicitPointer()) {
        expEmit->makeImplicitPointer(ctx, None);
      } else if (expEmit->isReference()) {
        expEmit->loadImplicitPointer(ctx->builder);
      }
      auto* uFn = expTy->asCore()->getUnaryOperator("#");
      return uFn->call(ctx, {expEmit->getLLVM()}, ctx->getMod());
    } else {
      ctx->Error("Core type " + ctx->highlightError(expTy->asCore()->getFullName()) +
                     " does not have the dereference operator #",
                 exp->fileRange);
    }
  } else {
    ctx->Error("Expression being dereferenced is of the incorrect type", exp->fileRange);
  }
  return nullptr;
}

Json Dereference::toJson() const {
  return Json()._("nodeType", "dereference")._("expression", exp->toJson())._("fileRange", fileRange);
}

} // namespace qat::ast