#include "./tuple_value.hpp"
#include "../../IR/logic.hpp"
#include "llvm/IR/DerivedTypes.h"

namespace qat::ast {

TupleValue::TupleValue(Vec<Expression*> _members, FileRange _fileRange) : Expression(_fileRange), members(_members) {}

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
    if (expMemTy
            ? (expMemTy->isSame(memRes->getType()) ||
               (memRes->getType()->isReference() && memRes->getType()->asReference()->getSubType()->isSame(expMemTy)))
            : true) {
      if (expMemTy ? !expMemTy->isSame(memRes->getType())
                   : (memRes->getType()->isReference() || memRes->isImplicitPointer())) {
        auto* refSubTy =
            memRes->getType()->isReference() ? memRes->getType()->asReference()->getSubType() : memRes->getType();
        if (refSubTy->isExpanded()) {
          auto* expSubTy   = refSubTy->asExpanded();
          bool  preferMove = mem->nodeType() == NodeType::moveExpression;
          if (!preferMove && expSubTy->hasCopyConstructor()) {
            if (mem->nodeType() == NodeType::copyExpression) {
              memRes = new IR::Value(ctx->builder.CreateLoad(expSubTy->getLLVMType(), memRes->getLLVM()), expSubTy,
                                     false, IR::Nature::temporary);
            } else {
              ctx->Warning("This expression is copied here. Please make copy explicit like " +
                               ctx->highlightWarning("copy(exp)"),
                           mem->fileRange);
              auto* newMemInst =
                  IR::Logic::newAlloca(ctx->getActiveFunction(), utils::unique_id(), refSubTy->getLLVMType());
              expSubTy->copyConstructValue(
                  ctx,
                  {new IR::Value(newMemInst, IR::ReferenceType::get(true, expSubTy, ctx), false, IR::Nature::temporary),
                   memRes},
                  ctx->getActiveFunction());
              memRes = new IR::Value(ctx->builder.CreateLoad(expSubTy->getLLVMType(), newMemInst), expSubTy, false,
                                     IR::Nature::temporary);
            }
          } else if (expSubTy->hasMoveConstructor()) {
            if (mem->nodeType() == NodeType::moveExpression) {
              memRes = new IR::Value(ctx->builder.CreateLoad(expSubTy->getLLVMType(), memRes->getLLVM()), expSubTy,
                                     false, IR::Nature::temporary);
            } else {
              ctx->Warning("This expression is moved here. Please make move explicit like " +
                               ctx->highlightWarning("move(exp)"),
                           mem->fileRange);
              auto* newMemInst =
                  IR::Logic::newAlloca(ctx->getActiveFunction(), utils::unique_id(), refSubTy->getLLVMType());
              expSubTy->moveConstructValue(
                  ctx,
                  {new IR::Value(newMemInst, IR::ReferenceType::get(true, expSubTy, ctx), false, IR::Nature::temporary),
                   memRes},
                  ctx->getActiveFunction());
              memRes = new IR::Value(ctx->builder.CreateLoad(expSubTy->getLLVMType(), newMemInst), expSubTy, false,
                                     IR::Nature::temporary);
            }
          } else {
            if (expSubTy->isTriviallyCopyable()) {
              memRes = new IR::Value(ctx->builder.CreateLoad(refSubTy->getLLVMType(), memRes->getLLVM()),
                                     memRes->getType()->asReference()->getSubType(), false, IR::Nature::temporary);
            } else {
              ctx->Error("This expression is a reference and could not be converted to a value since the type " +
                             ctx->highlightError(expSubTy->toString()) +
                             " does not have a copy or move constructor and is also not trivially copyable",
                         mem->fileRange);
            }
          }
        } else if (refSubTy->isOpaque()) {
          ctx->Error("This is a reference to " + ctx->highlightError(refSubTy->toString()) +
                         " which is an incomplete type with unknown size and structure",
                     mem->fileRange);
        } else {
          memRes = new IR::Value(
              ctx->builder.CreateLoad(memRes->getType()->asReference()->getSubType()->getLLVMType(), memRes->getLLVM()),
              memRes->getType()->asReference()->getSubType(), false, IR::Nature::temporary);
        }
      }
    } else if (expMemTy &&
               !((expMemTy->isReference() && expMemTy->asReference()->getSubType()->isSame(memRes->getType()) &&
                  memRes->isImplicitPointer()))) {
      ctx->Error("The type of this expression is " + ctx->highlightError(memRes->getType()->toString()) +
                     " which is not compatible with the expected type " + ctx->highlightError(expMemTy->toString()),
                 mem->fileRange);
    }
    tupleMemVals.push_back(memRes);
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
  return newLocal;
}

Json TupleValue::toJson() const {
  Vec<JsonValue> mems;
  for (auto mem : members) {
    mems.push_back(mem->toJson());
  }
  return Json()._("nodeType", "tupleValue")._("isPacked", isPacked)._("members", mems)._("fileRange", fileRange);
}

} // namespace qat::ast