#include "./address_of.hpp"
#include "llvm/Support/raw_ostream.h"

namespace qat::ast {

IR::Value* AddressOf::emit(IR::Context* ctx) {
  auto inst = instance->emit(ctx);
  if (inst->isReference() || inst->isImplicitPointer()) {
    auto subTy    = inst->isReference() ? inst->getType()->asReference()->getSubType() : inst->getType();
    bool isPtrVar = inst->isReference() ? inst->getType()->asReference()->isSubtypeVariable() : inst->isVariable();
    if (inst->isReference()) {
      inst->loadImplicitPointer(ctx->builder);
    }
    return new IR::Value(inst->getLLVM(),
                         IR::PointerType::get(isPtrVar, subTy, true, IR::PointerOwner::OfAnonymous(), false, ctx),
                         false, IR::Nature::temporary);
  } else {
    ctx->Error("The expression provided is of type " + ctx->highlightError(inst->getType()->toString()) +
                   ". It is not a reference, local or global value, so its address cannot be retrieved",
               fileRange);
  }
  return nullptr;
}

Json AddressOf::toJson() const {
  return Json()._("nodeType", "addressOf")._("instance", instance->toJson())._("fileRange", fileRange);
}

} // namespace qat::ast