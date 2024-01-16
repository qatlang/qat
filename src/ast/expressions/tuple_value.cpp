#include "./tuple_value.hpp"
#include "../../IR/logic.hpp"
#include "llvm/IR/DerivedTypes.h"

namespace qat::ast {

IR::Value* TupleValue::emit(IR::Context* ctx) {
  IR::TupleType* tupleTy = nullptr;
  if (isTypeInferred() || isLocalDecl()) {
    if (isLocalDecl() && !localValue->getType()->isTuple()) {
      ctx->Error("Expected expression of type " + ctx->highlightError(localValue->getType()->toString()) +
                     ", but found a tuple",
                 fileRange);
    }
    if (!inferredType->isTuple()) {
      ctx->Error("The inferred type for this tuple expression is " + ctx->highlightError(inferredType->toString()) +
                     ", which is not a tuple type",
                 fileRange);
    }
    tupleTy = isTypeInferred() ? inferredType->asTuple() : localValue->getType()->asTuple();
    if (inferredType->asTuple()->getSubTypeCount() != members.size()) {
      ctx->Error("Expected the type of this tuple to be " + ctx->highlightError(inferredType->toString()) + " with " +
                     ctx->highlightError(std::to_string(inferredType->asTuple()->getSubTypeCount())) +
                     " members. But " + ctx->highlightError(std::to_string(members.size())) + " values were provided",
                 fileRange);
    }
  }
  Vec<IR::QatType*> tupleMemTys;
  Vec<IR::Value*>   tupleMemVals;
  for (usize i = 0; i < members.size(); i++) {
    auto* expMemTy = tupleTy ? tupleTy->getSubtypeAt(i) : nullptr;
    auto* mem      = members.at(i);
    if (expMemTy) {
      if (mem->hasTypeInferrance()) {
        mem->asTypeInferrable()->setInferenceType(expMemTy->isReference() ? expMemTy->asReference()->getSubType()
                                                                          : expMemTy);
      }
    }
    auto* memRes = mem->emit(ctx);
    if (!tupleTy) {
      tupleMemTys.push_back(memRes->getType()->isReference() ? memRes->getType()->asReference()->getSubType()
                                                             : memRes->getType());
    }
    if (memRes->getType()->isReference()) {
      memRes->loadImplicitPointer(ctx->builder);
    }
    if (expMemTy) {
      if (expMemTy->isSame(memRes->getType())) {
        if (memRes->isImplicitPointer()) {
          auto memType = memRes->getType();
          if (memType->isTriviallyCopyable() || memType->isTriviallyMovable()) {
            auto* loadRes = ctx->builder.CreateLoad(memType->getLLVMType(), memRes->getLLVM());
            if (!memType->isTriviallyCopyable()) {
              if (!memRes->isVariable()) {
                ctx->Error("This expression does not have variability and hence cannot be trivially moved from",
                           mem->fileRange);
              }
              ctx->builder.CreateStore(llvm::Constant::getNullValue(memType->getLLVMType()), memRes->getLLVM());
            }
            tupleMemVals.push_back(new IR::Value(loadRes, memType, false, IR::Nature::temporary));
          } else {
            ctx->Error("This expression is of type " + ctx->highlightError(memType->toString()) +
                           " which is trivially copyable or movable. Please use " + ctx->highlightError("'copy") +
                           " or " + ctx->highlightError("'move") + " accordingly",
                       mem->fileRange);
          }
        } else {
          tupleMemVals.push_back(memRes);
        }
      } else if ((expMemTy->isReference() && expMemTy->asReference()->isSame(memRes->getType()) &&
                  (memRes->isImplicitPointer() &&
                   (expMemTy->asReference()->isSubtypeVariable() ? memRes->isVariable() : true))) ||
                 (expMemTy->isReference() && memRes->isReference() &&
                  expMemTy->asReference()->getSubType()->isSame(memRes->getType()->asReference()->getSubType()) &&
                  (expMemTy->asReference()->isSubtypeVariable() ? memRes->getType()->asReference()->isSubtypeVariable()
                                                                : true))) {
        if (memRes->isReference()) {
          memRes->loadImplicitPointer(ctx->builder);
        }
        tupleMemVals.push_back(memRes);
      } else if (memRes->isReference() && memRes->getType()->asReference()->getSubType()->isSame(expMemTy)) {
        memRes->loadImplicitPointer(ctx->builder);
        auto memType = memRes->getType()->asReference()->getSubType();
        if (memType->isTriviallyCopyable() || memType->isTriviallyMovable()) {
          auto* loadRes = ctx->builder.CreateLoad(memType->getLLVMType(), memRes->getLLVM());
          if (!memType->isTriviallyCopyable()) {
            if (!memRes->getType()->asReference()->isSubtypeVariable()) {
              ctx->Error("This expression is of type " + ctx->highlightError(memRes->getType()->toString()) +
                             " which is a reference without variability and hence cannot be trivially moved from",
                         mem->fileRange);
            }
          }
          tupleMemVals.push_back(new IR::Value(loadRes, memType, false, IR::Nature::temporary));
        } else {
          ctx->Error("This expression is a reference of type " + ctx->highlightError(memType->toString()) +
                         " which is not trivially copyable or movable. Please use " + ctx->highlightError("'copy") +
                         " or " + ctx->highlightError("'move") + " accordingly",
                     mem->fileRange);
        }
      } else {
        ctx->Error("Expected expression of type " + ctx->highlightError(expMemTy->toString()) +
                       ", but the provided expression is of type " + ctx->highlightError(memRes->getType()->toString()),
                   mem->fileRange);
      }
    } else {
      tupleMemVals.push_back(memRes);
    }
  }
  if (!tupleTy) {
    tupleTy = IR::TupleType::get(tupleMemTys, isPacked, ctx->llctx);
  }
  auto* newLocal = isLocalDecl()
                       ? localValue
                       : ctx->getActiveFunction()->getBlock()->newValue(
                             irName.has_value() ? irName.value().value : utils::unique_id(), tupleTy,
                             irName.has_value() ? isVar : true, irName.has_value() ? irName.value().range : fileRange);
  for (usize i = 0; i < tupleMemTys.size(); i++) {
    ctx->builder.CreateStore(tupleMemVals.at(i)->getLLVM(),
                             ctx->builder.CreateStructGEP(tupleTy->getLLVMType(), newLocal->getLLVM(), i));
  }
  return newLocal->toNewIRValue();
}

Json TupleValue::toJson() const {
  Vec<JsonValue> mems;
  for (auto mem : members) {
    mems.push_back(mem->toJson());
  }
  return Json()._("nodeType", "tupleValue")._("isPacked", isPacked)._("members", mems)._("fileRange", fileRange);
}

} // namespace qat::ast