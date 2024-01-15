#include "./cast.hpp"

namespace qat::ast {

Cast* Cast::Create(Expression* exp, QatType* dest, FileRange range) { return new Cast(exp, dest, range); }

Cast::Cast(Expression* _exp, QatType* _dest, FileRange _range)
    : Expression(_range), instance(_exp), destination(_dest) {}

IR::Value* Cast::emit(IR::Context* ctx) {
  auto inst   = instance->emit(ctx);
  auto srcTy  = inst->getType()->isReference() ? inst->getType()->asReference()->getSubType() : inst->getType();
  auto destTy = destination->emit(ctx);
  if (ctx->dataLayout.value().getTypeAllocSize(srcTy->getLLVMType()) ==
      ctx->dataLayout.value().getTypeAllocSize(destTy->getLLVMType())) {
    if (inst->isReference() || inst->isImplicitPointer()) {
      if (inst->isReference()) {
        inst->loadImplicitPointer(ctx->builder);
      }
      if (srcTy->isTriviallyCopyable() || srcTy->isTriviallyMovable()) {
        auto instllvm = inst->getLLVM();
        inst          = new IR::Value(ctx->builder.CreateLoad(srcTy->getLLVMType(), inst->getLLVM()), srcTy, false,
                                      IR::Nature::temporary);
        if (!srcTy->isTriviallyCopyable()) {
          if (inst->isReference() && !inst->getType()->asReference()->isSubtypeVariable()) {
            ctx->Error("This expression is a reference without variability and hence cannot be trivially moved from",
                       instance->fileRange);
          } else if (!inst->isVariable()) {
            ctx->Error("This expression does not have variability and hence cannot be trivially moved from", fileRange);
          }
          ctx->builder.CreateStore(llvm::Constant::getNullValue(srcTy->getLLVMType()), instllvm);
        }
      } else {
        ctx->Error(
            "The type of the expression is " + ctx->highlightError(srcTy->toString()) +
                " which is not trivially copyable or trivially movable, and hence casting cannot be performed. Try using " +
                ctx->highlightError("'copy") + " or " + ctx->highlightError("'move") + " accordingly",
            instance->fileRange);
      }
    }
    return new IR::Value(ctx->builder.CreateBitCast(inst->getLLVM(), destTy->getLLVMType()), destTy, false,
                         IR::Nature::temporary);
  } else {
    ctx->Error("The type of the expression is " + ctx->highlightError(srcTy->toString()) +
                   " with the allocation size of " +
                   ctx->highlightError(std::to_string(ctx->dataLayout.value().getTypeAllocSize(srcTy->getLLVMType())) +
                                       " bytes") +
                   " and the destination type is " + ctx->highlightError(destTy->toString()) +
                   " with the allocation size of " +
                   ctx->highlightError(std::to_string(ctx->dataLayout.value().getTypeAllocSize(destTy->getLLVMType())) +
                                       " bytes") +
                   ". The size of these types do not match and hence casting cannot be performed",
               fileRange);
  }
  return nullptr;
}

Json Cast::toJson() const {
  return Json()
      ._("nodeType", "cast")
      ._("expression", instance->toJson())
      ._("targetType", destination->toJson())
      ._("fileRange", fileRange);
}

} // namespace qat::ast