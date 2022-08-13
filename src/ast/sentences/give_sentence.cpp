#include "./give_sentence.hpp"
namespace qat::ast {

GiveSentence::GiveSentence(Maybe<Expression *> _given_expr,
                           utils::FileRange    _fileRange)
    : Sentence(std::move(_fileRange)), give_expr(_given_expr) {}

IR::Value *GiveSentence::emit(IR::Context *ctx) {
  // TODO - Implement destructor calls
  auto *fun = ctx->fn;
  if (fun->getType()->asFunction()->getReturnType()->typeKind() ==
      IR::TypeKind::Void) {
    if (give_expr.has_value()) {
      ctx->Error("Given value type of the function is void. Please remove this "
                 "unnecessary value",
                 give_expr.value()->fileRange);
    } else {
      return new IR::Value(ctx->builder.CreateRetVoid(),
                           IR::VoidType::get(ctx->llctx), false,
                           IR::Nature::pure);
    }
  } else {
    if (give_expr.has_value()) {
      auto *retVal  = give_expr.value()->emit(ctx);
      auto *retType = fun->getType()->asFunction()->getReturnType();
      if (retType->isReference()
              ? retType->asReference()->getSubType()->isSame(retVal->getType())
              : retType->isSame(retVal->getType())) {
        if (retVal->isImplicitPointer() && !retType->isReference()) {
          retVal->loadImplicitPointer(ctx->builder);
        }
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