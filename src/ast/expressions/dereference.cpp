#include "./dereference.hpp"

namespace qat::ast {

Dereference::Dereference(Expression* _exp, utils::FileRange _fileRange)
    : Expression(std::move(_fileRange)), exp(_exp) {}

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
      expEmit = new IR::Value(ctx->builder.CreateLoad(expTy->getLLVMType(), expEmit->getLLVM()), expTy, false,
                              IR::Nature::temporary);
    } else {
      expEmit->loadImplicitPointer(ctx->builder);
    }
    return new IR::Value(
        expEmit->getLLVM(),
        IR::ReferenceType::get(expTy->asPointer()->isSubtypeVariable(), expTy->asPointer()->getSubType(), ctx->llctx),
        false, IR::Nature::temporary);
  } else if (expTy->isCoreType()) {
    if (expTy->asCore()->hasUnaryOperator("#")) {
      if (expEmit->isImplicitPointer() || expEmit->isReference()) {
        auto* uFn = expTy->asCore()->getUnaryOperator("#");
        return uFn->call(ctx, {expEmit->getLLVM()}, ctx->getMod());
      } else {
        ctx->Error("Expression is of the invalid type", exp->fileRange);
      }
    } else {
      ctx->Error("Core type " + ctx->highlightError(expTy->asCore()->getFullName()) + " does not have operator #",
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