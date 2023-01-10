#include "./self_member.hpp"
#include "../../IR/types/void.hpp"

namespace qat::ast {

SelfMember::SelfMember(Identifier _name, FileRange _fileRange)
    : Expression(std::move(_fileRange)), name(std::move(_name)) {}

IR::Value* SelfMember::emit(IR::Context* ctx) {
  SHOW("Emitting self member at: " << (Json)fileRange)
  auto* fun = ctx->fn;
  if (fun->isMemberFunction()) {
    auto* mFn = (IR::MemberFunction*)fun;
    if (mFn->isStaticFunction()) {
      ctx->Error("Function " + ctx->highlightError(mFn->getFullName()) +
                     " is a static function and cannot use the expression " + ctx->highlightError("''" + name.value),
                 fileRange);
    }
    auto* expandedTy = mFn->getParentType();
    if (expandedTy->isCoreType() && expandedTy->asCore()->hasMember(name.value)) {
      auto* cTy   = expandedTy->asCore();
      auto  index = cTy->getIndexOf(name.value).value();
      auto* mem   = cTy->getMemberAt(index);
      mem->addMention(name.range);
      auto* selfVal = mFn->getBlock()->getValue("''");
      auto* res     = new IR::Value(
          ctx->builder.CreateStructGEP(expandedTy->getLLVMType(), selfVal->getLLVM(), index),
          IR::ReferenceType::get(selfVal->getType()->asReference()->isSubtypeVariable(), mem->type, ctx->llctx), false,
          IR::Nature::temporary);
      while (res->getType()->isReference() && res->getType()->asReference()->getSubType()->isReference()) {
        res = new IR::Value(
            ctx->builder.CreateLoad(res->getType()->asReference()->getSubType()->getLLVMType(), res->getLLVM()),
            res->getType()->asReference()->getSubType(), false, IR::Nature::temporary);
      }
      return res;
    } else if (expandedTy->isMix()) {
      ctx->Error("Cannot access member fields by name since the parent type " +
                     ctx->highlightError(expandedTy->getFullName()) + " of this member function is a mix type",
                 fileRange);
    } else {
      ctx->Error("The parent type of this member function " + ctx->highlightError(expandedTy->getFullName()) +
                     " does not have a "
                     "member field named " +
                     ctx->highlightError(name.value),
                 fileRange);
    }
  } else {
    ctx->Error(ctx->highlightError("''" + name.value) + " cannot be used here because the function " +
                   ctx->highlightError(fun->getFullName()) + " is not a member function of any type",
               fileRange);
  }
  return nullptr;
}

Json SelfMember::toJson() const { return Json()._("nodeType", "selfMember")._("fileRange", fileRange); }

} // namespace qat::ast