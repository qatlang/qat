#include "./self_member.hpp"

namespace qat::ast {

SelfMember::SelfMember(String _name, utils::FileRange _fileRange)
    : Expression(std::move(_fileRange)), name(std::move(_name)) {}

IR::Value *SelfMember::emit(IR::Context *ctx) {
  SHOW("Emitting self member")
  auto *fun = ctx->fn;
  if (fun->isMemberFunction()) {
    auto *mFn = (IR::MemberFunction *)fun;
    if (mFn->isStaticFunction()) {
      ctx->Error("Function " + ctx->highlightError(mFn->getFullName()) +
                     " is a static function and cannot use the expression " +
                     ctx->highlightError("''" + name),
                 fileRange);
    }
    auto *cTy = mFn->getParentType();
    if (cTy->hasMember(name)) {
      auto  index   = cTy->getIndexOf(name).value();
      auto *mem     = cTy->getMemberAt(index);
      auto *selfVal = mFn->getBlock()->getValue("''");
      return new IR::Value(
          ctx->builder.CreateStructGEP(cTy->getLLVMType(), ctx->selfVal, index),
          IR::ReferenceType::get(
              selfVal->getType()->asPointer()->isSubtypeVariable() &&
                  mem->variability,
              mem->type, ctx->llctx),
          false, IR::Nature::temporary);
    } else {
      ctx->Error("The parent type of this member function does not have a "
                 "member named " +
                     ctx->highlightError(name),
                 fileRange);
    }
  } else {
    ctx->Error(ctx->highlightError("''" + name) +
                   " cannot be used here because the function " +
                   ctx->highlightError(fun->getFullName()) +
                   " is not a member function of any type",
               fileRange);
  }
  return nullptr;
}

nuo::Json SelfMember::toJson() const {
  return nuo::Json()._("nodeType", "selfMember")._("fileRange", fileRange);
}

} // namespace qat::ast