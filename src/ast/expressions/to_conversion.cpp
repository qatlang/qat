#include "./to_conversion.hpp"
#include "../../IR/control_flow.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/c_type.hpp"
#include "../../IR/types/string_slice.hpp"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Casting.h"

namespace qat::ast {

IR::Value* ToConversion::emit(IR::Context* ctx) {
  auto* val    = source->emit(ctx);
  auto* destTy = destinationType->emit(ctx);
  if (val->getType()->isSame(destTy)) {
    if (val->getType()->isDefinition() || destTy->isDefinition()) {
      return new IR::Value(val->getLLVM(), destTy, false, IR::Nature::temporary);
    } else {
      ctx->Warning("Unnecessary conversion here. The expression is already of type " +
                       ctx->highlightWarning(destTy->toString()) + ". Please remove this to avoid clutter.",
                   fileRange);
      return val;
    }
  } else {
    auto* typ     = val->getType();
    auto  loadRef = [&] {
      if (val->isReference()) {
        val->loadImplicitPointer(ctx->builder);
        val = new IR::Value(
            ctx->builder.CreateLoad(val->getType()->asReference()->getSubType()->getLLVMType(), val->getLLVM()),
            val->getType()->asReference()->getSubType(), false, IR::Nature::temporary);
        typ = val->getType();
      } else {
        val->loadImplicitPointer(ctx->builder);
      }
    };
    auto valType = val->isReference() ? val->getType()->asReference()->getSubType() : val->getType();
    if (valType->isCType()) {
      valType = valType->asCType()->getSubType();
    }
    if (valType->isPointer()) {
      if (destTy->isPointer() || (destTy->isCType() && destTy->asCType()->isCPointer())) {
        loadRef();
        auto targetTy = destTy->isCType() ? destTy->asCType()->getSubType()->asPointer() : destTy->asPointer();
        if (!valType->asPointer()->getOwner().isSame(targetTy->getOwner()) && !targetTy->getOwner().isAnonymous()) {
          ctx->Error(
              "This change of ownership of the pointer type is not allowed. Pointers with valid ownership can only be converted to anonymous ownership",
              fileRange);
        }
        if (valType->asPointer()->isNullable() != targetTy->asPointer()->isNullable()) {
          if (valType->asPointer()->isNullable()) {
            auto  fun           = ctx->getActiveFunction();
            auto* currBlock     = fun->getBlock();
            auto* nullTrueBlock = new IR::Block(fun, currBlock);
            auto* restBlock     = new IR::Block(fun, currBlock->getParent());
            restBlock->linkPrevBlock(currBlock);
            ctx->builder.CreateCondBr(
                ctx->builder.CreateICmpEQ(
                    ctx->builder.CreatePtrDiff(valType->asPointer()->getSubType()->isTypeSized()
                                                   ? valType->asPointer()->getSubType()->getLLVMType()
                                                   : llvm::Type::getInt8Ty(ctx->llctx),
                                               valType->asPointer()->isMulti()
                                                   ? ctx->builder.CreateExtractValue(val->getLLVM(), {0u})
                                                   : val->getLLVM(),
                                               llvm::ConstantPointerNull::get(llvm::PointerType::get(
                                                   valType->asPointer()->getSubType()->isTypeSized()
                                                       ? valType->asPointer()->getSubType()->getLLVMType()
                                                       : llvm::Type::getInt8Ty(ctx->llctx),
                                                   ctx->dataLayout.value().getProgramAddressSpace()))),
                    llvm::ConstantInt::get(IR::CType::getPtrDiff(ctx)->getLLVMType(), 0u, true)),
                nullTrueBlock->getBB(), restBlock->getBB());
            nullTrueBlock->setActive(ctx->builder);
            IR::Logic::panicInFunction(
                fun,
                {IR::StringSliceType::create_value(
                    ctx, "This is a null-pointer and hence cannot be converted to the non-nullable pointer type " +
                             destTy->toString())},
                {}, fileRange, ctx);
            (void)IR::addBranch(ctx->builder, restBlock->getBB());
            restBlock->setActive(ctx->builder);
          }
        }
        if (!valType->asPointer()->getSubType()->isSame(targetTy->getSubType())) {
          ctx->Error(
              "The value to be converted is of type " + ctx->highlightError(typ->toString()) +
                  " but the destination type is " + ctx->highlightError(destTy->toString()) +
                  ". The subtype of the pointer types do not match and conversion between them is not allowed. Use casting instead",
              fileRange);
        }
        if (valType->asPointer()->isMulti() != targetTy->asPointer()->isMulti()) {
          if (valType->asPointer()->isMulti()) {
            return new IR::Value(ctx->builder.CreateExtractValue(val->getLLVM(), {0u}), destTy, false,
                                 IR::Nature::temporary);
          } else {
            auto newVal = ctx->getActiveFunction()->getBlock()->newValue(utils::unique_id(), destTy, false, fileRange);
            ctx->builder.CreateStore(val->getLLVM(),
                                     ctx->builder.CreateStructGEP(destTy->getLLVMType(), newVal->getLLVM(), 0u));
            ctx->builder.CreateStore(llvm::ConstantInt::get(IR::CType::getUsize(ctx)->getLLVMType(), 1u),
                                     ctx->builder.CreateStructGEP(destTy->getLLVMType(), newVal->getLLVM(), 1u));
            return newVal->toNewIRValue();
          }
        }
        if (valType->asPointer()->isMulti()) {
          val->getLLVM()->mutateType(destTy->getLLVMType());
          return new IR::Value(val->getLLVM(), destTy, false, IR::Nature::temporary);
        } else {
          return new IR::Value(ctx->builder.CreatePointerCast(val->getLLVM(), destTy->getLLVMType()), destTy, false,
                               IR::Nature::temporary);
        }
      } else if (destTy->isCType() && (destTy->asCType()->isIntPtr() || destTy->asCType()->isIntPtrUnsigned())) {
        loadRef();
        return new IR::Value(ctx->builder.CreateBitCast(valType->asPointer()->isMulti()
                                                            ? ctx->builder.CreateExtractValue(val->getLLVM(), {0u})
                                                            : val->getLLVM(),
                                                        destTy->getLLVMType()),
                             destTy, false, IR::Nature::temporary);
      } else {
        ctx->Error("Pointer conversion to type " + ctx->highlightError(destTy->toString()) + " is not supported",
                   fileRange);
      }
    } else if (valType->isStringSlice()) {
      if (destTy->isCType() && destTy->asCType()->isCString()) {
        loadRef();
        if (val->isLLVMConstant()) {
          return new IR::PrerunValue(
              llvm::dyn_cast<llvm::ConstantStruct>(val->getLLVMConstant())->getAggregateElement(0u), destTy->asCType());
        } else {
          return new IR::Value(ctx->builder.CreateExtractValue(val->getLLVM(), {0u}), destTy, false,
                               IR::Nature::temporary);
        }
      } else if (destTy->isPointer() && destTy->asPointer()->getSubType()->isUnsignedInteger() &&
                 destTy->asPointer()->isNullable() && destTy->asPointer()->getOwner().isAnonymous() &&
                 (destTy->asPointer()->getSubType()->asUnsignedInteger()->getBitwidth() == 8u)) {
        loadRef();
        if (destTy->asPointer()->isMulti()) {
          val->getLLVM()->mutateType(destTy->getLLVMType());
          return new IR::Value(val->getLLVM(), destTy, false, IR::Nature::temporary);
        } else {
          return new IR::Value(ctx->builder.CreateExtractValue(val->getLLVM(), {0u}), destTy, false,
                               IR::Nature::temporary);
        }
      } else {
        ctx->Error("Conversion from " + ctx->highlightError(typ->toString()) + " to " +
                       ctx->highlightError(destTy->toString()) + " is not supported",
                   fileRange);
      }
    } else if (valType->isInteger()) {
      if (destTy->isInteger() || (destTy->isCType() && destTy->asCType()->getSubType()->isInteger())) {
        loadRef();
        return new IR::Value(ctx->builder.CreateIntCast(val->getLLVM(), destTy->getLLVMType(), true), destTy,
                             val->isVariable(), val->getNature());
      } else if (destTy->isUnsignedInteger() ||
                 (destTy->isCType() && destTy->asCType()->getSubType()->isUnsignedInteger())) {
        loadRef();
        ctx->Warning("Conversion from signed integer to unsigned integers can be lossy", fileRange);
        if (valType->asInteger()->getBitwidth() == destTy->asUnsignedInteger()->getBitwidth()) {
          return new IR::Value(ctx->builder.CreateIntCast(val->getLLVM(), destTy->getLLVMType(), true), destTy, false,
                               IR::Nature::pure);
        } else {
          return new IR::Value(ctx->builder.CreateIntCast(val->getLLVM(), destTy->getLLVMType(), true), destTy, false,
                               IR::Nature::temporary);
        }
      } else if (destTy->isFloat() || (destTy->isCType() && destTy->asCType()->isFloat())) {
        loadRef();
        return new IR::Value(ctx->builder.CreateSIToFP(val->getLLVM(), destTy->getLLVMType()), destTy, false,
                             IR::Nature::temporary);
      } else {
        ctx->Error("Conversion from " + ctx->highlightError(typ->toString()) + " to " +
                       ctx->highlightError(destTy->toString()) + " is not supported",
                   fileRange);
      }
    } else if (valType->isUnsignedInteger()) {
      if (destTy->isUnsignedInteger() || (destTy->isCType() && destTy->asCType()->getSubType()->isUnsignedInteger())) {
        loadRef();
        return new IR::Value(ctx->builder.CreateIntCast(val->getLLVM(), destTy->getLLVMType(), false), destTy, false,
                             IR::Nature::temporary);
      } else if (destTy->isInteger() || (destTy->isCType() && destTy->asCType()->getSubType()->isInteger())) {
        loadRef();
        ctx->Warning("Conversion from unsigned integer to signed integers can be lossy", fileRange);
        if (typ->asUnsignedInteger()->getBitwidth() == destTy->asInteger()->getBitwidth()) {
          return new IR::Value(val->getLLVM(), destTy, false, IR::Nature::temporary);
        } else {
          return new IR::Value(ctx->builder.CreateIntCast(val->getLLVM(), destTy->getLLVMType(), true), destTy, false,
                               IR::Nature::temporary);
        }
      } else if (destTy->isFloat() || (destTy->isCType() && destTy->asCType()->getSubType()->isFloat())) {
        loadRef();
        return new IR::Value(ctx->builder.CreateUIToFP(val->getLLVM(), destTy->getLLVMType()), destTy, false,
                             IR::Nature::temporary);
      } else {
        ctx->Error("Conversion from " + ctx->highlightError(typ->toString()) + " to " +
                       ctx->highlightError(destTy->toString()) + " is not supported",
                   fileRange);
      }
    } else if (valType->isFloat()) {
      if (destTy->isFloat() || (destTy->isCType() && destTy->asCType()->getSubType()->isFloat())) {
        loadRef();
        return new IR::Value(ctx->builder.CreateFPCast(val->getLLVM(), destTy->getLLVMType()), destTy, false,
                             IR::Nature::temporary);
      } else if (destTy->isInteger() || (destTy->isCType() && destTy->asCType()->getSubType()->isInteger())) {
        loadRef();
        return new IR::Value(ctx->builder.CreateFPToSI(val->getLLVM(), destTy->getLLVMType()), destTy, false,
                             IR::Nature::temporary);
      } else if (destTy->isUnsignedInteger() ||
                 (destTy->isCType() && destTy->asCType()->getSubType()->isUnsignedInteger())) {
        loadRef();
        return new IR::Value(ctx->builder.CreateFPToUI(val->getLLVM(), destTy->getLLVMType()), destTy, false,
                             IR::Nature::temporary);
      } else {
        ctx->Error("Conversion from " + ctx->highlightError(typ->toString()) + " to " +
                       ctx->highlightError(destTy->toString()) + " is not supported",
                   fileRange);
      }
    } else if (valType->isChoice()) {
      if ((destTy->isInteger() || destTy->isUnsignedInteger()) && valType->asChoice()->hasProvidedType() &&
          (destTy->isInteger() == valType->asChoice()->getProvidedType()->isInteger())) {
        auto* intTy = valType->asChoice()->getProvidedType()->asInteger();
        if (intTy->getBitwidth() == destTy->asInteger()->getBitwidth()) {
          loadRef();
          return new IR::Value(val->getLLVM(), destTy, false, IR::Nature::temporary);
        } else {
          ctx->Error("The underlying type of the choice type " + ctx->highlightError(valType->toString()) + " is " +
                         ctx->highlightError(intTy->toString()) + ", but the type for conversion is " +
                         ctx->highlightError(destTy->toString()),
                     fileRange);
        }
      } else {
        ctx->Error("The underlying type of the choice type " + ctx->highlightError(valType->toString()) + " is " +
                       ctx->highlightError(valType->asChoice()->getProvidedType()->toString()) +
                       ", but the target type for conversion is " + ctx->highlightError(destTy->toString()),
                   fileRange);
      }
    } else {
      ctx->Error("Conversion from " + ctx->highlightError(typ->toString()) + " to " +
                     ctx->highlightError(destTy->toString()) + " is not supported",
                 fileRange);
    }
  }
}

Json ToConversion::toJson() const {
  return Json()
      ._("nodeType", "toConversion")
      ._("instance", source->toJson())
      ._("targetType", destinationType->toJson())
      ._("fileRange", fileRange);
}

} // namespace qat::ast