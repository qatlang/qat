#include "./index_access.hpp"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/Casting.h"

namespace qat::ast {

IndexAccess::IndexAccess(Expression* _instance, Expression* _index, utils::FileRange _fileRange)
    : Expression(std::move(_fileRange)), instance(_instance), index(_index) {}

IR::Value* IndexAccess::emit(IR::Context* ctx) {
  auto* inst     = instance->emit(ctx);
  auto* instType = inst->getType();
  auto* ind      = index->emit(ctx);
  auto* indType  = ind->getType();
  auto* zero64   = llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), 0u);
  if (inst->getType()->isReference()) {
    instType = instType->asReference()->getSubType();
  }
  // TODO - Update when supporting operators for core types
  if (instType->isPointer() || instType->isArray() || instType->isTuple()) {
    if (!instType->isArray() && !instType->isPointer()) {
      inst->loadImplicitPointer(ctx->builder);
    }
    ind->loadImplicitPointer(ctx->builder);
    if (ind->getType()->isReference()) {
      indType = indType->asReference()->getSubType();
    }
    if (ind->getType()->isUnsignedInteger() || ind->getType()->isInteger() ||
        (ind->getType()->isReference() && (ind->getType()->asReference()->getSubType()->isUnsignedInteger() ||
                                           ind->getType()->asReference()->getSubType()->isInteger()))) {
      if (inst->getType()->isReference() && (inst->getType()->asReference()->getSubType()->isPointer() &&
                                             !inst->getType()->asReference()->getSubType()->asPointer()->isMulti())) {
        SHOW("Instance for member access is a Reference to: " << inst->getType()->toString())
        inst = new IR::Value(ctx->builder.CreateLoad(instType->getLLVMType(), inst->getLLVM()), instType,
                             inst->getType()->asReference()->isSubtypeVariable(),
                             inst->getType()->asReference()->isSubtypeVariable() ? IR::Nature::assignable
                                                                                 : IR::Nature::temporary);
      }
      if (ind->getType()->isReference()) {
        ind = new IR::Value(ctx->builder.CreateLoad(indType->getLLVMType(), ind->getLLVM()), indType,
                            ind->getType()->asReference()->isSubtypeVariable(),
                            ind->getType()->asReference()->isSubtypeVariable() ? IR::Nature::assignable
                                                                               : IR::Nature::temporary);
      }
      if (!instType->isTuple()) {
        if (instType->isPointer()) {
          if (!instType->asPointer()->isMulti()) {
            ctx->Error("Only multi pointers can be indexed into", fileRange);
          }
          Vec<llvm::Value*> idxs;
          idxs.push_back(ind->getLLVM());
          return new IR::Value(
              ctx->builder.CreateInBoundsGEP(
                  instType->asPointer()->getSubType()->getLLVMType(),
                  ctx->builder.CreateLoad(
                      llvm::PointerType::get(instType->asPointer()->getSubType()->getLLVMType(), 0u),
                      ctx->builder.CreateStructGEP(instType->getLLVMType(), inst->getLLVM(), 0u)),
                  idxs),
              IR::ReferenceType::get(inst->isVariable(), instType->asPointer()->getSubType(), ctx->llctx),
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
                  firstElem,
                  IR::ReferenceType::get(inst->isVariable(), instType->asArray()->getElementType(), ctx->llctx),
                  inst->isVariable(), inst->isVariable() ? IR::Nature::assignable : IR::Nature::temporary);
            }
          }
          SHOW("Got first element, getting specific element")
          return new IR::Value(
              ctx->builder.CreateInBoundsGEP(inst->getType()->getLLVMType()->getArrayElementType(), firstElem,
                                             ind->getLLVM()),
              IR::ReferenceType::get(inst->isVariable(), instType->asArray()->getElementType(), ctx->llctx),
              inst->isVariable(), inst->isVariable() ? IR::Nature::assignable : IR::Nature::temporary);
        }
      } else {
        if (llvm::isa<llvm::ConstantInt>(ind->getLLVM())) {
          // NOLINTBEGIN(clang-analyzer-core.CallAndMessage)
          auto constantIndex = *(llvm::dyn_cast<llvm::ConstantInt>(ind->getLLVM())->getValue().getRawData());
          // NOLINTEND(clang-analyzer-core.CallAndMessage)
          if (constantIndex < instType->asTuple()->getSubTypeCount()) {
            return new IR::Value(ctx->builder.CreateStructGEP(instType->getLLVMType(), inst->getLLVM(), constantIndex),
                                 instType->asTuple()->getSubtypeAt(constantIndex), inst->isVariable(),
                                 inst->getNature());
          } else {
            ctx->Error("The provided index exceeds the maximum value for this "
                       "tuple type. Remember that tuples are zero indexed. The "
                       "index value starts at " +
                           ctx->highlightError("0") + " and ends at " + ctx->highlightError("length - 1"),
                       index->fileRange);
          }
        } else {
          ctx->Error("Tuple members can only be accessed with indices that are "
                     "unsigned integer literals",
                     index->fileRange);
        }
      }
    } else {
      ctx->Error("The index is of type " + ind->getType()->toString() +
                     ". It should be a signed or an unsigned integer",
                 index->fileRange);
    }
  } else if (instType->isStringSlice()) {
    ind->loadImplicitPointer(ctx->builder);
    if (!ind->getType()->isInteger() && !ind->getType()->isUnsignedInteger()) {
      ctx->Error("Invalid type for the index of string slice", fileRange);
    }
    // FIXME - POSSIBLY THROW ERROR AT RUNTIME IF BOUNDS ARE EXCEEDED
    if (inst->isImplicitPointer() || inst->isReference()) {
      auto* strData =
          ctx->builder.CreateStructGEP(IR::StringSliceType::get(ctx->llctx)->getLLVMType(), inst->getLLVM(), 0);
      SHOW("Got string data")
      SHOW("Got first element")
      return new IR::Value(ctx->builder.CreateInBoundsGEP(
                               llvm::Type::getInt8Ty(ctx->llctx),
                               ctx->builder.CreateLoad(llvm::Type::getInt8Ty(ctx->llctx)->getPointerTo(), strData),
                               Vec<llvm::Value*>({ind->getLLVM()})),
                           IR::ReferenceType::get(inst->isReference()
                                                      ? inst->getType()->asReference()->isSubtypeVariable()
                                                      : inst->isVariable(),
                                                  IR::UnsignedType::get(8, ctx->llctx), ctx->llctx),
                           false, IR::Nature::temporary);
    } else {
      ctx->Error("Invalid value for string slice and hence string cannot be indexed", fileRange);
    }
  } else if (instType->isCString()) {
    ind->loadImplicitPointer(ctx->builder);
    inst->loadImplicitPointer(ctx->builder);
    auto* instVal = inst->getLLVM();
    if (inst->isReference()) {
      instVal = ctx->builder.CreateLoad(instType->getLLVMType(), inst->getLLVM());
    }
    bool isVarExp = inst->isReference() ? inst->getType()->asReference()->isSubtypeVariable() : inst->isVariable();
    return new IR::Value(ctx->builder.CreateInBoundsGEP(llvm::Type::getInt8Ty(ctx->llctx), instVal, {ind->getLLVM()}),
                         IR::ReferenceType::get(isVarExp, IR::UnsignedType::get(8, ctx->llctx), ctx->llctx), false,
                         IR::Nature::temporary);
  } else if (instType->isCoreType()) {
    auto*               cTy     = instType->asCore();
    IR::MemberFunction* opFn    = nullptr;
    IR::Value*          operand = nullptr;
    if (cTy->hasBinaryOperator("[]", indType)) {
      opFn    = cTy->getBinaryOperator("[]", indType);
      operand = ind;
    } else if (indType->isReference() && cTy->hasBinaryOperator("[]", indType->asReference()->getSubType())) {
      opFn    = cTy->getBinaryOperator("[]", indType->asReference()->getSubType());
      operand = new IR::Value(ind->getLLVM(), indType->asReference()->getSubType(), false, IR::Nature::temporary);
    } else if (cTy->hasBinaryOperator("[]", IR::ReferenceType::get(true, indType, ctx->llctx))) {
      if (ind->isImplicitPointer()) {
        if (!ind->isVariable()) {
          ctx->Error("The [] operator of core type " + ctx->highlightError(cTy->getFullName()) +
                         " expects a variable reference, but the provided "
                         "expression is not a variable",
                     index->fileRange);
        }
        // FIXME
      } else {
      }
    }
    if (!inst->isImplicitPointer() && !inst->isReference()) {
      inst->makeImplicitPointer(ctx, None);
    }

  } else {
    ctx->Error("The expression cannot be used for index access", instance->fileRange);
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