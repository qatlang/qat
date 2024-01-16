#include "./self.hpp"

namespace qat::ast {

IR::Value* Self::emit(IR::Context* ctx) {
  if (getExpectedKind() == ExpressionKind::assignable) {
    ctx->Error("Self is not assignable", fileRange);
  }
  if (ctx->hasActiveFunction()) {
    if (ctx->getActiveFunction()->isMemberFunction()) {
      auto* mFn = (IR::MemberFunction*)ctx->getActiveFunction();
      return new IR::Value(mFn->getFirstBlock()->getValue("''")->getLLVM(),
                           IR::ReferenceType::get(mFn->isVariationFunction(), mFn->getParentType(), ctx), false,
                           IR::Nature::temporary);
    } else {
      // FIXME - Support extension functions that will have non-core type parent
      ctx->Error("The current function is not a member function", fileRange);
    }
  } else {
    ctx->Error("No active function in the current scope", fileRange);
  }
}

Json Self::toJson() const { return Json()._("nodeType", "self")._("fileRange", fileRange); }

} // namespace qat::ast