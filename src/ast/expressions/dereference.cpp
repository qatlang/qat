#include "./dereference.hpp"

namespace qat::ast {

Dereference::Dereference(Expression *_exp, utils::FileRange _fileRange)
    : Expression(std::move(_fileRange)), exp(_exp) {}

IR::Value *Dereference::emit(IR::Context *ctx) {
  auto *expEmit = exp->emit(ctx);
  auto *expTy   = expEmit->getType();
  if (expTy->isReference()) {
    expEmit->loadImplicitPointer(ctx->builder);
    expTy = expTy->asReference()->getSubType();
  }
  if (expTy->isPointer()) {
    if (expEmit->isReference()) {
      expEmit->loadImplicitPointer(ctx->builder);
      expEmit = new IR::Value(
          ctx->builder.CreateLoad(expTy->getLLVMType(), expEmit->getLLVM()),
          expTy, false, IR::Nature::temporary);
    } else {
      expEmit->loadImplicitPointer(ctx->builder);
    }
    return new IR::Value(
        expEmit->getLLVM(),
        IR::ReferenceType::get(expTy->asPointer()->isSubtypeVariable(),
                               expTy->asPointer()->getSubType(), ctx->llctx),
        false, IR::Nature::temporary);
  } else {
    ctx->Error("Only pointers can be dereferenced", exp->fileRange);
  }
  return nullptr;
}

nuo::Json Dereference::toJson() const {
  return nuo::Json()
      ._("nodeType", "dereference")
      ._("expression", exp->toJson())
      ._("fileRange", fileRange);
}

} // namespace qat::ast