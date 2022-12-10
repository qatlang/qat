#include "./self.hpp"

namespace qat::ast {

Self::Self(FileRange _fileRange) : Expression(std::move(_fileRange)) {}

IR::Value* Self::emit(IR::Context* ctx) {
  if (getExpectedKind() == ExpressionKind::assignable) {
    ctx->Error("Self is not assignable", fileRange);
  }
  if (ctx->fn) {
    if (ctx->fn->isMemberFunction()) {
      auto* mFn = (IR::MemberFunction*)ctx->fn;
      if (ctx->selfVal) {
        return new IR::Value(ctx->selfVal,
                             IR::ReferenceType::get(mFn->isVariationFunction(), mFn->getParentType(), ctx->llctx),
                             false, IR::Nature::temporary);
      } else {
        ctx->Error("Invalid value for self in the current scope", fileRange);
      }
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