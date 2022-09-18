#include "./give_sentence.hpp"
namespace qat::ast {

GiveSentence::GiveSentence(Maybe<Expression *> _given_expr,
                           utils::FileRange    _fileRange)
    : Sentence(std::move(_fileRange)), give_expr(_given_expr) {}

IR::Value *GiveSentence::emit(IR::Context *ctx) {
  auto *fun = ctx->fn;
  if (fun->getType()->asFunction()->getReturnType()->typeKind() ==
      IR::TypeKind::Void) {
    if (give_expr.has_value()) {
      ctx->Error("Given value type of the function is void. Please remove this "
                 "unnecessary value",
                 give_expr.value()->fileRange);
    } else {
      if (fun->isMemberFunction() &&
          ((IR::MemberFunction *)fun)->getMemberFnType() ==
              IR::MemberFnType::destructor) {
        auto *mFn = (IR::MemberFunction *)fun;
        auto *cTy = mFn->getParentType();
        for (usize i = 0; i < cTy->getMemberCount(); i++) {
          if (cTy->getMemberAt(i)->type->isCoreType()) {
            auto *memPtr = ctx->builder.CreateStructGEP(
                mFn->getParentType()->getLLVMType(), ctx->selfVal, i);
            auto *destructor = cTy->getDestructor();
            (void)destructor->call(ctx, {memPtr}, ctx->getMod());
          }
        }
      }
      Vec<IR::LocalValue *> locals;
      fun->getBlock()->collectLocalValues(locals);
      for (auto *loc : locals) {
        if (loc->getType()->isCoreType()) {
          auto *cTy        = loc->getType()->asCore();
          auto *destructor = cTy->getDestructor();
          (void)destructor->call(ctx, {loc->getAlloca()}, ctx->getMod());
        }
      }
      locals.clear();
      return new IR::Value(ctx->builder.CreateRetVoid(),
                           IR::VoidType::get(ctx->llctx), false,
                           IR::Nature::pure);
    }
  } else {
    if (give_expr.has_value()) {
      auto *retVal  = give_expr.value()->emit(ctx);
      auto *retType = fun->getType()->asFunction()->getReturnType();
      if (retType->isSame(retVal->getType()) ||
          (retType->isReference() &&
           retType->asReference()->getSubType()->isSame(retVal->getType()) &&
           retVal->isImplicitPointer()) ||
          (retVal->getType()->isReference() &&
           retVal->getType()->asReference()->getSubType()->isSame(retType))) {
        if (retVal->getType()->isReference() && !(retType->isReference())) {
          retVal = new IR::Value(ctx->builder.CreateLoad(retType->getLLVMType(),
                                                         retVal->getLLVM()),
                                 retType, false, IR::Nature::temporary);
        } else if (retVal->isImplicitPointer() && !retType->isReference()) {
          retVal->loadImplicitPointer(ctx->builder);
        }
        if (retType->isReference() && retVal->isLocalToFn()) {
          ctx->Warning("Returning reference to a local value. The value "
                       "pointed to by the reference might be destroyed before "
                       "the function call is complete",
                       fileRange);
        }
        Vec<IR::LocalValue *> locals;
        fun->getBlock()->collectLocalValues(locals);
        for (auto *loc : locals) {
          if (loc->getType()->isCoreType()) {
            auto *cTy        = loc->getType()->asCore();
            auto *destructor = cTy->getDestructor();
            (void)destructor->call(ctx, {loc->getAlloca()}, ctx->getMod());
          }
        }
        locals.clear();
        return new IR::Value(ctx->builder.CreateRet(retVal->getLLVM()),
                             retVal->getType(), false, IR::Nature::pure);
      } else {
        ctx->Error(
            "Given value type of the function is " +
                fun->getType()->asFunction()->getReturnType()->toString() +
                ", but the provided value in the give sentence is " +
                retVal->getType()->toString(),
            give_expr.value()->fileRange);
      }
    } else {
      ctx->Error("No value is provided for the give sentence. Please provide a "
                 "value of the appropriate type",
                 fileRange);
    }
  }
}

nuo::Json GiveSentence::toJson() const {
  return nuo::Json()
      ._("nodeType", "giveSentence")
      ._("hasValue", give_expr.has_value())
      ._("value",
         give_expr.has_value() ? give_expr.value()->toJson() : nuo::Json())
      ._("fileRange", fileRange);
}

} // namespace qat::ast