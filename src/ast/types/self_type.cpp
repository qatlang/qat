#include "./self_type.hpp"
#include "../../IR/types/reference.hpp"

namespace qat::ast {

IR::QatType* SelfType::emit(IR::Context* ctx) {
  if (ctx->hasActiveFunction() && ctx->getActiveFunction()->isMemberFunction()) {
    if (isJustType) {
      return ((IR::MemberFunction*)ctx->getActiveFunction())->getParentType();
    } else {
      if (canBeSelfInstance) {
        return IR::ReferenceType::get(isVarRef, ((IR::MemberFunction*)ctx->getActiveFunction())->getParentType(), ctx);
      } else {
        ctx->Error("The " + ctx->highlightError("''") +
                       " type can only be used as the return type in member functions and variation functions",
                   fileRange);
      }
    }
  } else if (ctx->hasActiveType()) {
    if (isJustType) {
      return ctx->getActiveType();
    } else {
      if (canBeSelfInstance) {
        return IR::ReferenceType::get(isVarRef, ctx->getActiveType(), ctx);
      } else {
        ctx->Error("The " + ctx->highlightError("''") +
                       " type can only be used as return type in member functions and variation functions",
                   fileRange);
      }
    }
  } else {
    ctx->Error("No parent type found in this scope.", fileRange);
  }
}

Json SelfType::toJson() const {
  return Json()._("nodeType", "selfType")._("isJustType", isJustType)._("fileRange", fileRange);
}

String SelfType::toString() const { return isJustType ? "self" : "''"; }

} // namespace qat::ast