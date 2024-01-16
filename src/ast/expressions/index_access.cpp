#include "./index_access.hpp"
#include "../../IR/control_flow.hpp"
#include "../../IR/logic.hpp"
#include "llvm/Analysis/ConstantFolding.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/Casting.h"

namespace qat::ast {

IR::Value* IndexAccess::emit(IR::Context* ctx) {
  auto* inst     = instance->emit(ctx);
  auto* instType = inst->getType();
  if (instType->isReference()) {
    instType = instType->asReference()->getSubType();
  }
  if (index->hasTypeInferrance()) {
    if (instType->isTuple()) {
      index->asTypeInferrable()->setInferenceType(IR::UnsignedType::get(32u, ctx));
    } else if (instType->isArray()) {
      index->asTypeInferrable()->setInferenceType(IR::UnsignedType::get(64u, ctx));
    } else if (instType->isPointer() || instType->isStringSlice()) {
      index->asTypeInferrable()->setInferenceType(IR::CType::getUsize(ctx));
    }
  }
  auto* ind     = index->emit(ctx);
  auto* indType = ind->getType();
  auto* zero64  = llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), 0u);
  if (instType->isTuple()) {
    if (indType->isUnsignedInteger() && ind->isPrerunValue()) {
      if (indType->asUnsignedInteger()->getBitwidth() > 32u) {
        ctx->Error(
            ctx->highlightError(indType->toString()) +
                " is an unsupported type of the index for accessing tuple members. The maximum allowed bitwidth is 32",
            index->fileRange);
      }
      auto indPre = (u32)(*llvm::cast<llvm::ConstantInt>(
                               indType->asUnsignedInteger()->getBitwidth() < 32u
                                   ? llvm::ConstantFoldConstant(
                                         llvm::ConstantExpr::getIntegerCast(ind->getLLVMConstant(),
                                                                            llvm::Type::getInt32Ty(ctx->llctx), false),
                                         ctx->dataLayout.value())
                                   : ind->getLLVMConstant())
                               ->getValue()
                               .getRawData());
      if (indPre >= instType->asTuple()->getSubTypeCount()) {
        ctx->Error("The index value is " + ctx->highlightError(std::to_string(indPre)) + " which is not less than " +
                       ctx->highlightError(std::to_string(instType->asTuple()->getSubTypeCount())) +
                       ", the number of members in this tuple",
                   index->fileRange);
      }
      if (inst->isReference() || inst->isImplicitPointer()) {
        bool isMemVar = inst->isReference() ? inst->getType()->asReference()->isSubtypeVariable() : inst->isVariable();
        if (inst->isReference()) {
          inst->loadImplicitPointer(ctx->builder);
        }
        return new IR::Value(ctx->builder.CreateStructGEP(instType->getLLVMType(), inst->getLLVM(), indPre),
                             IR::ReferenceType::get(isMemVar, instType->asTuple()->getSubtypeAt(indPre), ctx), false,
                             IR::Nature::temporary);
      } else if (inst->isPrerunValue()) {
        if (llvm::isa<llvm::ConstantExpr>(inst->getLLVMConstant())) {
          return new IR::PrerunValue(llvm::ConstantFoldConstant(inst->getLLVMConstant(), ctx->dataLayout.value()),
                                     instType->asTuple()->getSubtypeAt(indPre));
        } else {
          return new IR::PrerunValue(inst->getLLVMConstant()->getAggregateElement(indPre),
                                     instType->asTuple()->getSubtypeAt(indPre));
        }
      } else {
        return new IR::Value(ctx->builder.CreateExtractValue(inst->getLLVM(), {indPre}),
                             instType->asTuple()->getSubtypeAt(indPre), false, IR::Nature::temporary);
      }
    } else {
      ctx->Error("Tuple members can only be accessed using prerun unsigned integers of the " +
                     ctx->highlightError("u32") + " type",
                 index->fileRange);
    }
  } else if (instType->isPointer() || instType->isArray()) {
    ind->loadImplicitPointer(ctx->builder);
    if (ind->getType()->isReference()) {
      indType = indType->asReference()->getSubType();
    }
    if (indType->isUnsignedInteger() || (indType->isCType() && indType->asCType()->isUsize())) {
      if (inst->getType()->isReference() && (inst->getType()->asReference()->getSubType()->isPointer() &&
                                             !inst->getType()->asReference()->getSubType()->asPointer()->isMulti())) {
        SHOW("Instance for member access is a Reference to: "
             << inst->getType()->asReference()->getSubType()->toString())
        inst = new IR::Value(ctx->builder.CreateLoad(instType->getLLVMType(), inst->getLLVM()), instType,
                             inst->getType()->asReference()->isSubtypeVariable(), IR::Nature::temporary);
      }
      if (ind->getType()->isReference()) {
        ind = new IR::Value(ctx->builder.CreateLoad(indType->getLLVMType(), ind->getLLVM()), indType,
                            ind->getType()->asReference()->isSubtypeVariable(),
                            ind->getType()->asReference()->isSubtypeVariable() ? IR::Nature::assignable
                                                                               : IR::Nature::temporary);
      }
      if (instType->isPointer()) {
        if (!instType->asPointer()->isMulti()) {
          ctx->Error("Only multi pointers can be indexed into", fileRange);
        }
        auto* lenExceedTrueBlock = new IR::Block(ctx->getActiveFunction(), ctx->getActiveFunction()->getBlock());
        auto* restBlock = new IR::Block(ctx->getActiveFunction(), ctx->getActiveFunction()->getBlock()->getParent());
        restBlock->linkPrevBlock(ctx->getActiveFunction()->getBlock());
        auto ptrLen =
            ctx->builder.CreateLoad(IR::CType::getUsize(ctx)->getLLVMType(),
                                    ctx->builder.CreateStructGEP(instType->getLLVMType(), inst->getLLVM(), 1u));
        ctx->builder.CreateCondBr(ctx->builder.CreateICmpUGE(ind->getLLVM(), ptrLen), lenExceedTrueBlock->getBB(),
                                  restBlock->getBB());
        lenExceedTrueBlock->setActive(ctx->builder);
        IR::Logic::panicInFunction(ctx->getActiveFunction(),
                                   {IR::StringSliceType::create_value(ctx, "The index is "), ind,
                                    IR::StringSliceType::create_value(
                                        ctx, " which is not less than the length of the multipointer, which is "),
                                    new IR::Value(ptrLen, IR::CType::getUsize(ctx), false, IR::Nature::temporary)},
                                   {}, index->fileRange, ctx);
        (void)IR::addBranch(ctx->builder, restBlock->getBB());
        restBlock->setActive(ctx->builder);
        Vec<llvm::Value*> idxs;
        idxs.push_back(ind->getLLVM());
        return new IR::Value(
            ctx->builder.CreateInBoundsGEP(
                instType->asPointer()->getSubType()->getLLVMType(),
                ctx->builder.CreateLoad(llvm::PointerType::get(instType->asPointer()->getSubType()->getLLVMType(),
                                                               ctx->dataLayout->getProgramAddressSpace()),
                                        ctx->builder.CreateStructGEP(instType->getLLVMType(), inst->getLLVM(), 0u)),
                idxs),
            IR::ReferenceType::get(instType->asPointer()->isSubtypeVariable(), instType->asPointer()->getSubType(),
                                   ctx),
            instType->asPointer()->isSubtypeVariable(),
            instType->asPointer()->isSubtypeVariable() ? IR::Nature::assignable : IR::Nature::temporary);
      } else {
        SHOW("Getting first element")
        auto* firstElem =
            ctx->builder.CreateInBoundsGEP(inst->getType()->getLLVMType(), inst->getLLVM(), {zero64, zero64});
        if (llvm::isa<llvm::ConstantInt>(ind->getLLVM())) {
          SHOW("Index Access : Index is a constant")
          // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
          auto indConst = *(llvm::dyn_cast<llvm::ConstantInt>(ind->getLLVM())->getValue().getRawData());
          if (indConst == 0) {
            SHOW("Returning the first element from inbounds")
            return new IR::Value(
                firstElem, IR::ReferenceType::get(inst->isVariable(), instType->asArray()->getElementType(), ctx),
                inst->isVariable(), inst->isVariable() ? IR::Nature::assignable : IR::Nature::temporary);
          }
        }
        SHOW("Got first element, getting specific element")
        return new IR::Value(ctx->builder.CreateInBoundsGEP(inst->getType()->getLLVMType()->getArrayElementType(),
                                                            firstElem, ind->getLLVM()),
                             IR::ReferenceType::get(inst->isVariable(), instType->asArray()->getElementType(), ctx),
                             inst->isVariable(), inst->isVariable() ? IR::Nature::assignable : IR::Nature::temporary);
      }
    } else {
      ctx->Error("The index is of type " + ind->getType()->toString() + ". It should be of the " +
                     ctx->highlightError("usize") + " type",
                 index->fileRange);
    }
  } else if (instType->isStringSlice()) {
    if (ind->isReference() || ind->isImplicitPointer()) {
      ind->loadImplicitPointer(ctx->builder);
      if (ind->isReference()) {
        ind     = new IR::Value(ctx->builder.CreateLoad(ind->isReference()
                                                            ? ind->getType()->asReference()->getSubType()->getLLVMType()
                                                            : ind->getType()->getLLVMType(),
                                                        ind->getLLVM()),
                            ind->isReference() ? ind->getType()->asReference()->getSubType() : ind->getType(), false,
                                IR::Nature::temporary);
        indType = ind->getType();
      }
    }
    if (!ind->getType()->isCType() || (ind->getType()->isCType() && !ind->getType()->asCType()->isUsize())) {
      ctx->Error(ctx->highlightError(ind->getType()->toString()) +
                     " is an invalid type for the index of string slice. The index should be of type " +
                     ctx->highlightError("usize"),
                 fileRange);
    }
    if (inst->isImplicitPointer() || inst->isReference()) {
      if (inst->isReference()) {
        inst->loadImplicitPointer(ctx->builder);
      }
      auto  usizeTy            = IR::CType::getUsize(ctx);
      auto* strTy              = IR::StringSliceType::get(ctx);
      auto* strLen             = ctx->builder.CreateLoad(usizeTy->getLLVMType(),
                                                         ctx->builder.CreateStructGEP(strTy->getLLVMType(), inst->getLLVM(), 1u));
      auto* currBlock          = ctx->getActiveFunction()->getBlock();
      auto* lenExceedTrueBlock = new IR::Block(ctx->getActiveFunction(), currBlock);
      auto* restBlock          = new IR::Block(ctx->getActiveFunction(), currBlock->getParent());
      restBlock->linkPrevBlock(currBlock);
      ctx->builder.CreateCondBr(ctx->builder.CreateICmpUGE(ind->getLLVM(), strLen), lenExceedTrueBlock->getBB(),
                                restBlock->getBB());
      lenExceedTrueBlock->setActive(ctx->builder);
      IR::Logic::panicInFunction(
          ctx->getActiveFunction(),
          {IR::StringSliceType::create_value(ctx, "Index for string slice is "), ind,
           IR::StringSliceType::create_value(ctx, " which is not less than its length, which is "),
           new IR::Value(strLen, IR::UnsignedType::get(64u, ctx), false, IR::Nature::temporary)},
          {}, fileRange, ctx);
      (void)IR::addBranch(ctx->builder, restBlock->getBB());
      restBlock->setActive(ctx->builder);
      auto* strData = ctx->builder.CreateStructGEP(IR::StringSliceType::get(ctx)->getLLVMType(), inst->getLLVM(), 0u);
      SHOW("Got string data")
      SHOW("Got first element")
      return new IR::Value(ctx->builder.CreateInBoundsGEP(
                               llvm::Type::getInt8Ty(ctx->llctx),
                               ctx->builder.CreateLoad(llvm::Type::getInt8Ty(ctx->llctx)->getPointerTo(), strData),
                               Vec<llvm::Value*>({ind->getLLVM()})),
                           IR::ReferenceType::get(inst->isReference()
                                                      ? inst->getType()->asReference()->isSubtypeVariable()
                                                      : inst->isVariable(),
                                                  IR::UnsignedType::get(8u, ctx), ctx),
                           false, IR::Nature::temporary);
    } else if (inst->isPrerunValue() && ind->isPrerunValue()) {
      if (llvm::cast<llvm::ConstantInt>(
              llvm::ConstantFoldConstant(llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_ULT,
                                                                     ind->getLLVMConstant(),
                                                                     inst->getLLVMConstant()->getAggregateElement(1u)),
                                         ctx->dataLayout.value()))
              ->getValue()
              .getBoolValue()) {
        return new IR::PrerunValue(
            llvm::ConstantInt::get(
                llvm::Type::getInt8Ty(ctx->llctx),
                llvm::cast<llvm::ConstantDataArray>(inst->getLLVMConstant()->getAggregateElement(0u)->getOperand(0u))
                    ->getRawDataValues()
                    .str()
                    .at(*llvm::cast<llvm::ConstantInt>(ind->getLLVMConstant())->getValue().getRawData())),
            IR::UnsignedType::get(8u, ctx));
      } else {
        ctx->Error("The provided index is " +
                       ctx->highlightError(ind->getType()->toPrerunGenericString((IR::PrerunValue*)ind).value()) +
                       " which is not less than the length of the string slice, which is " +
                       ctx->highlightError(
                           ind->getType()
                               ->toPrerunGenericString(new IR::PrerunValue(
                                   inst->getLLVMConstant()->getAggregateElement(1u), IR::UnsignedType::get(64u, ctx)))
                               .value()),
                   index->fileRange);
      }
    } else {
      // FIXME - Add check
      auto* strTy              = IR::StringSliceType::get(ctx);
      auto* strLen             = ctx->builder.CreateLoad(llvm::Type::getInt64Ty(ctx->llctx),
                                                         ctx->builder.CreateStructGEP(strTy->getLLVMType(), inst->getLLVM(), 1u));
      auto* currBlock          = ctx->getActiveFunction()->getBlock();
      auto* lenExceedTrueBlock = new IR::Block(ctx->getActiveFunction(), currBlock);
      auto* restBlock          = new IR::Block(ctx->getActiveFunction(), currBlock->getParent());
      restBlock->linkPrevBlock(currBlock);
      ctx->builder.CreateCondBr(ctx->builder.CreateICmpUGE(ind->getLLVM(), strLen), lenExceedTrueBlock->getBB(),
                                restBlock->getBB());
      lenExceedTrueBlock->setActive(ctx->builder);
      IR::Logic::panicInFunction(
          ctx->getActiveFunction(),
          {IR::StringSliceType::create_value(ctx, "Index of string slice is not less than its length")}, {}, fileRange,
          ctx);
      (void)IR::addBranch(ctx->builder, restBlock->getBB());
      restBlock->setActive(ctx->builder);
      return new IR::Value(
          ctx->builder.CreateInBoundsGEP(
              llvm::Type::getInt8Ty(ctx->llctx), ctx->builder.CreateExtractValue(inst->getLLVM(), {0u}),
              {llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), 0u), ind->getLLVM()}),
          IR::ReferenceType::get(inst->isReference() ? inst->getType()->asReference()->isSubtypeVariable()
                                                     : inst->isVariable(),
                                 IR::UnsignedType::get(8u, ctx), ctx),
          false, IR::Nature::temporary);
    }
  } else if (instType->isCType() && instType->asCType()->isCString()) {
    ind->loadImplicitPointer(ctx->builder);
    inst->loadImplicitPointer(ctx->builder);
    auto* instVal = inst->getLLVM();
    if (inst->isReference()) {
      instVal = ctx->builder.CreateLoad(instType->getLLVMType(), inst->getLLVM());
    }
    bool isVarExp = inst->isReference() ? inst->getType()->asReference()->isSubtypeVariable() : inst->isVariable();
    return new IR::Value(ctx->builder.CreateInBoundsGEP(llvm::Type::getInt8Ty(ctx->llctx), instVal, {ind->getLLVM()}),
                         IR::ReferenceType::get(isVarExp, IR::CType::getChar(ctx), ctx), false, IR::Nature::temporary);
  } else if (instType->isExpanded()) {
    auto*               eTy      = instType->asExpanded();
    IR::MemberFunction* opFn     = nullptr;
    IR::Value*          operand  = nullptr;
    Maybe<String>       localID  = inst->getLocalID();
    bool                isVarExp = inst->isReference() ? inst->getType()->asReference()->isSubtypeVariable()
                                                       : (inst->isImplicitPointer() ? inst->isVariable() : true);
    SHOW("Index access: isVarExp = " << isVarExp)
    if (!indType->isReference() &&
        ((isVarExp && eTy->hasVariationBinaryOperator(
                          "[]", {ind->isImplicitPointer() ? Maybe<bool>(ind->isVariable()) : None, indType})) ||
         eTy->hasNormalBinaryOperator("[]",
                                      {ind->isImplicitPointer() ? Maybe<bool>(ind->isVariable()) : None, indType}))) {
      if ((isVarExp && eTy->hasVariationBinaryOperator("[]", {None, indType})) ||
          eTy->hasNormalBinaryOperator("[]", {None, indType})) {
        opFn = (isVarExp && eTy->hasVariationBinaryOperator("[]", {None, indType}))
                   ? eTy->getVariationBinaryOperator("[]", {None, indType})
                   : eTy->getNormalBinaryOperator("[]", {None, indType});
        if (ind->isImplicitPointer()) {
          if (indType->isTriviallyCopyable() || indType->isTriviallyMovable()) {
            auto indVal = new IR::Value(ctx->builder.CreateLoad(indType->getLLVMType(), ind->getLLVM()), indType, false,
                                        IR::Nature::temporary);
            if (!indType->isTriviallyCopyable()) {
              if (!ind->isVariable()) {
                ctx->Error("This expression does not have variability and hence cannot be trivially moved from",
                           index->fileRange);
              }
              ctx->builder.CreateStore(llvm::Constant::getNullValue(indType->getLLVMType()), ind->getLLVM());
            }
            ind = indVal;
          } else {
            ctx->Error("This expression is of type " + ctx->highlightError(indType->toString()) +
                           " which cannot be trivially copied and trivially moved from",
                       index->fileRange);
          }
        }
        operand = ind;
      } else {
        opFn    = (isVarExp && eTy->hasVariationBinaryOperator(
                                "[]", {ind->isImplicitPointer() ? Maybe<bool>(ind->isVariable()) : None, indType}))
                      ? eTy->getVariationBinaryOperator(
                         "[]", {ind->isImplicitPointer() ? Maybe<bool>(ind->isVariable()) : None, indType})
                      : eTy->getNormalBinaryOperator(
                         "[]", {ind->isImplicitPointer() ? Maybe<bool>(ind->isVariable()) : None, indType});
        operand = ind;
      }
    } else if (indType->isReference() &&
               ((isVarExp && eTy->hasVariationBinaryOperator("[]", {None, indType->asReference()->getSubType()})) ||
                eTy->hasNormalBinaryOperator("[]", {None, indType->asReference()->getSubType()}))) {
      opFn = (isVarExp && eTy->hasVariationBinaryOperator("[]", {None, indType->asReference()->getSubType()}))
                 ? eTy->getVariationBinaryOperator("[]", {None, indType->asReference()->getSubType()})
                 : eTy->getNormalBinaryOperator("[]", {None, indType->asReference()->getSubType()});
      ind->loadImplicitPointer(ctx->builder);
      auto* indSubType = indType->asReference()->getSubType();
      if (indSubType->isTriviallyCopyable() || indSubType->isTriviallyMovable()) {
        auto indVal = new IR::Value(ctx->builder.CreateLoad(indSubType->getLLVMType(), ind->getLLVM()), indSubType,
                                    false, IR::Nature::temporary);
        if (!indType->isTriviallyCopyable()) {
          if (!indType->asReference()->isSubtypeVariable()) {
            ctx->Error("This expression is a reference without variability and hence cannot be trivially moved from",
                       index->fileRange);
          }
          ctx->builder.CreateStore(llvm::Constant::getNullValue(indSubType->getLLVMType()), ind->getLLVM());
        }
        ind = indVal;
      } else {
        ctx->Error("This expression is a reference to type " + ctx->highlightError(indSubType->toString()) +
                       " which cannot be trivially copied or trivially moved. Please use " +
                       ctx->highlightError("'copy") + " or " + ctx->highlightError("'move") + " accordingly",
                   index->fileRange);
      }
      operand = ind;
    } else {
      ctx->Error("No matching [] operator found in type " + eTy->toString() + " for index type " +
                     ctx->highlightError(indType->toString()),
                 index->fileRange);
    }
    if (inst->isReference()) {
      inst->loadImplicitPointer(ctx->builder);
    }
    return opFn->call(ctx, {inst->getLLVM(), operand->getLLVM()}, localID, ctx->getMod());
  } else {
    ctx->Error("The expression of type " + instType->toString() + " cannot be used for index access",
               instance->fileRange);
  }
}

Json IndexAccess::toJson() const {
  return Json()
      ._("nodeType", "memberIndexAccess")
      ._("instance", instance->toJson())
      ._("index", index->toJson())
      ._("fileRange", fileRange);
}

} // namespace qat::ast