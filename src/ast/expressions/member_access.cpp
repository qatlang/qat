#include "./member_access.hpp"
#include "../../IR/types/future.hpp"
#include "../../IR/types/maybe.hpp"
#include "../../IR/types/string_slice.hpp"
#include "entity.hpp"
#include "llvm/Analysis/ConstantFolding.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/Casting.h"

namespace qat::ast {

IR::Value* MemberAccess::emit(IR::Context* ctx) {
  // NOLINTBEGIN(readability-magic-numbers, clang-analyzer-core.CallAndMessage)
  if (isExpSelf) {
    if (ctx->getActiveFunction()->isMemberFunction()) {
      auto* memFn = (IR::MemberFunction*)ctx->getActiveFunction();
      if (memFn->isStaticFunction()) {
        ctx->Error("This is a static member function and hence cannot access members of the parent instance",
                   fileRange);
      }
    } else {
      ctx->Error(
          "The parent function is not a member function of any type and hence cannot access members on the parent instance",
          fileRange);
    }
  } else {
    if (instance->nodeType() == NodeType::SELF) {
      ctx->Error("Do not use this syntax for accessing members of the parent instance. Use " +
                     ctx->highlightError(String("''") + (isPointerAccess ? "->" : "") +
                                         (isVariationAccess.has_value() && isVariationAccess.value()
                                              ? "var:"
                                              : (isVariationAccess.has_value() ? "const:" : "")) +
                                         name.value) +
                     " instead",
                 fileRange);
    }
  }
  auto* inst     = isExpSelf ? ctx->getActiveFunction()->getFirstBlock()->getValue("''") : instance->emit(ctx);
  auto* instType = inst->getType();
  bool  isVar    = inst->isVariable();
  if (isPointerAccess) {
    if (instType->isReference()) {
      inst->loadImplicitPointer(ctx->builder);
      isVar    = instType->asReference()->isSubtypeVariable();
      instType = instType->asReference()->getSubType();
    }
    if (instType->isPointer()) {
      if (inst->getType()->isReference()) {
        inst = new IR::Value(ctx->builder.CreateLoad(instType->getLLVMType(), inst->getLLVM()), instType, false,
                             IR::Nature::temporary);
      } else {
        inst->loadImplicitPointer(ctx->builder);
      }
      auto* ptrTy = instType->asPointer();
      if (!ptrTy->isNonNullable()) {
        ctx->Error("The expression is of pointer type " + ctx->highlightError(ptrTy->toString()) +
                       " which is a nullable pointer type -> cannot be used here",
                   fileRange);
      }
      if (ptrTy->isMulti()) {
        ctx->Error("The expression is of multi-pointer type " + ctx->highlightError(ptrTy->toString()) +
                       " and -> cannot be used here",
                   fileRange);
      }
      isVar    = ptrTy->isSubtypeVariable();
      instType = ptrTy->getSubType();
    } else {
      ctx->Error("The expression is of type " + ctx->highlightError(instType->toString()) +
                     " which is not a pointer type, and hence -> cannot be used here",
                 fileRange);
    }
  } else {
    if (instType->isReference()) {
      inst->loadImplicitPointer(ctx->builder);
      isVar    = instType->asReference()->isSubtypeVariable();
      instType = instType->asReference()->getSubType();
    }
  }
  if (instType->isArray()) {
    if (name.value == "length") {
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), instType->asArray()->getLength()),
          // NOLINTNEXTLINE(readability-magic-numbers)
          IR::IntegerType::get(64u, ctx));
    } else {
      ctx->Error("Invalid name for member access " + ctx->highlightError(name.value) + " for expression with type " +
                     ctx->highlightError(instType->toString()),
                 name.range);
    }
  } else if (instType->isStringSlice()) {
    if (name.value == "length") {
      if (inst->isLLVMConstant() && !isPointerAccess) {
        return new IR::PrerunValue(inst->getLLVMConstant()->getAggregateElement(1u), IR::CType::getUsize(ctx));
      } else if (!inst->isReference() && !inst->isImplicitPointer() && !inst->isLLVMConstant() && !isPointerAccess) {
        return new IR::Value(ctx->builder.CreateExtractValue(inst->getLLVM(), {1u}), IR::CType::getUsize(ctx), false,
                             IR::Nature::temporary);
      } else {
        return new IR::Value(
            ctx->builder.CreateStructGEP(IR::StringSliceType::get(ctx)->getLLVMType(), inst->getLLVM(), 1u),
            IR::ReferenceType::get(false, IR::CType::getUsize(ctx), ctx), false, IR::Nature::temporary);
      }
    } else if (name.value == "data") {
      if (inst->isLLVMConstant() && !isPointerAccess) {
        return new IR::PrerunValue(inst->getLLVMConstant()->getAggregateElement(0u),
                                   IR::PointerType::get(false, IR::UnsignedType::get(8u, ctx), false,
                                                        IR::PointerOwner::OfAnonymous(), false, ctx));
      } else if (!inst->isReference() && !inst->isImplicitPointer() && !inst->isLLVMConstant() && !isPointerAccess) {
        return new IR::Value(ctx->builder.CreateExtractValue(inst->getLLVM(), {0u}),
                             IR::PointerType::get(false, IR::UnsignedType::get(8u, ctx), false,
                                                  IR::PointerOwner::OfAnonymous(), false, ctx),
                             false, IR::Nature::temporary);
      } else {
        SHOW("String slice is an implicit pointer or a reference or pointer")
        return new IR::Value(
            ctx->builder.CreateStructGEP(IR::StringSliceType::get(ctx)->getLLVMType(), inst->getLLVM(), 0u),
            IR::ReferenceType::get(false,
                                   IR::PointerType::get(false, IR::UnsignedType::get(8u, ctx),
                                                        false, // NOLINT(readability-magic-numbers)
                                                        IR::PointerOwner::OfAnonymous(), false, ctx),
                                   ctx),
            false, IR::Nature::temporary);
      }
    } else {
      ctx->Error("Invalid name for member access: " + ctx->highlightError(name.value) + " for expression of type " +
                     instType->toString(),
                 name.range);
    }
  } else if (instType->isFuture()) {
    if (!inst->isReference() && !inst->isImplicitPointer() && !inst->isLLVMConstant() && !isPointerAccess) {
      inst = inst->makeLocal(ctx, None, instance->fileRange);
    }
    if (name.value == "isDone") {
      return new IR::Value(
          ctx->builder.CreateLoad(
              llvm::Type::getInt1Ty(ctx->llctx),
              ctx->builder.CreatePointerCast(
                  ctx->builder.CreateInBoundsGEP(
                      llvm::Type::getInt64Ty(ctx->llctx),
                      ctx->builder.CreateLoad(
                          llvm::Type::getInt64Ty(ctx->llctx)
                              ->getPointerTo(ctx->dataLayout.value().getProgramAddressSpace()),
                          ctx->builder.CreateStructGEP(instType->asFuture()->getLLVMType(), inst->getLLVM(), 1u)),
                      {llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), 1u)}),
                  llvm::Type::getInt1Ty(ctx->llctx)->getPointerTo(ctx->dataLayout.value().getProgramAddressSpace())),
              true),
          IR::UnsignedType::getBool(ctx), false, IR::Nature::temporary);
    } else if (name.value == "isNotDone") {
      return new IR::Value(
          ctx->builder.CreateICmpEQ(
              ctx->builder.CreateLoad(
                  llvm::Type::getInt1Ty(ctx->llctx),
                  ctx->builder.CreatePointerCast(
                      ctx->builder.CreateInBoundsGEP(
                          llvm::Type::getInt64Ty(ctx->llctx),
                          ctx->builder.CreateLoad(
                              llvm::Type::getInt64Ty(ctx->llctx)
                                  ->getPointerTo(ctx->dataLayout.value().getProgramAddressSpace()),
                              ctx->builder.CreateStructGEP(instType->asFuture()->getLLVMType(), inst->getLLVM(), 1u)),
                          {llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), 1u)}),
                      llvm::Type::getInt1Ty(ctx->llctx)
                          ->getPointerTo(ctx->dataLayout.value().getProgramAddressSpace())),
                  true),
              llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 0u)),
          IR::UnsignedType::getBool(ctx), false, IR::Nature::temporary);
    } else {
      ctx->Error("Invalid name " + ctx->highlightError(name.value) + " for member access for type " +
                     ctx->highlightError(instType->toString()),
                 name.range);
    }
  } else if (instType->isMaybe()) {
    if (!inst->isImplicitPointer() && !inst->isReference() && !inst->isLLVMConstant() && !isPointerAccess) {
      inst = inst->makeLocal(ctx, None, instance->fileRange);
    }
    if (name.value == "hasValue") {
      if (inst->isLLVMConstant()) {
        return new IR::PrerunValue(llvm::cast<llvm::ConstantInt>(inst->getLLVMConstant()->getAggregateElement(0u)),
                                   IR::UnsignedType::getBool(ctx));
      } else {
        return new IR::Value(
            ctx->builder.CreateICmpEQ(
                ctx->builder.CreateLoad(llvm::Type::getInt1Ty(ctx->llctx),
                                        ctx->builder.CreateStructGEP(instType->getLLVMType(), inst->getLLVM(), 0u)),
                llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u)),
            IR::UnsignedType::getBool(ctx), false, IR::Nature::temporary);
      }
    } else if (name.value == "hasNoValue") {
      if (inst->isLLVMConstant()) {
        return new IR::PrerunValue(
            llvm::ConstantFoldConstant(
                llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_EQ,
                                            inst->getLLVMConstant()->getAggregateElement(0u),
                                            llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 0u)),
                ctx->dataLayout.value()),
            IR::UnsignedType::getBool(ctx));
      } else {
        return new IR::Value(
            ctx->builder.CreateICmpEQ(
                ctx->builder.CreateLoad(llvm::Type::getInt1Ty(ctx->llctx),
                                        ctx->builder.CreateStructGEP(instType->getLLVMType(), inst->getLLVM(), 0u)),
                llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 0u)),
            IR::UnsignedType::getBool(ctx), false, IR::Nature::temporary);
      }
    } else {
      ctx->Error("Invalid name " + ctx->highlightError(name.value) + " for member access of type " +
                     ctx->highlightError(instType->toString()),
                 fileRange);
    }
  } else if (instType->isExpanded()) {
    if ((instType->isCoreType() && !instType->asCore()->hasMember(name.value)) &&
        ((isVariationAccess.has_value() && isVariationAccess.value())
             ? !instType->asExpanded()->hasVariationFn(name.value)
             : !instType->asExpanded()->hasNormalMemberFn(name.value))) {
      ctx->Error("Core type " + ctx->highlightError(instType->asCore()->toString()) +
                     " does not have a member field, member function or variation function named " +
                     ctx->highlightError(name.value) + ". Please check the logic",
                 name.range);
    }
    auto* eTy = instType->asExpanded();
    if (eTy->isCoreType() && eTy->asCore()->hasMember(name.value)) {
      if (isVariationAccess.has_value() && isVariationAccess.value()) {
        ctx->Error(ctx->highlightError(name.value) + " is a member field of type " +
                       ctx->highlightError(eTy->getFullName()) +
                       " and hence variation access cannot be used. Please change " +
                       ctx->highlightError("'var:" + name.value) + " to " + ctx->highlightError("'" + name.value),
                   fileRange);
      }
      auto* mem = eTy->asCore()->getMemberAt(instType->asCore()->getIndexOf(name.value).value());
      mem->addMention(name.range);
      if (isExpSelf) {
        auto* mFn = (IR::MemberFunction*)ctx->getActiveFunction();
        if (mFn->isConstructor()) {
          if (!mFn->isMemberInitted(eTy->asCore()->getMemberIndex(name.value))) {
            auto mem = eTy->asCore()->getMember(name.value);
            if (mem->defaultValue.has_value()) {
              ctx->Error(
                  "Member field " + ctx->highlightError(name.value) + " of parent type " +
                      ctx->highlightError(eTy->toString()) +
                      " is not initialised yet and hence cannot be used. The field has a default value provided, which will be used to initialise it only at the end of this constructor",
                  fileRange);
            } else {
              ctx->Error("Member field " + ctx->highlightError(name.value) + " of parent type " +
                             ctx->highlightError(eTy->toString()) +
                             " has not been initialised yet and hence cannot be used",
                         fileRange);
            }
          }
        } else {
          mFn->addUsedMember(mem->name.value);
        }
      }
      if (!mem->visibility.isAccessible(ctx->getAccessInfo())) {
        ctx->Error("Member " + ctx->highlightError(name.value) + " of core type " +
                       ctx->highlightError(eTy->getFullName()) + " is not accessible here",
                   fileRange);
      }
      if (!inst->isImplicitPointer() && !inst->getType()->isReference() && !inst->isLLVMConstant() &&
          !isPointerAccess) {
        inst = inst->makeLocal(ctx, None, instance->fileRange);
      }
      if (inst->isLLVMConstant() && !isPointerAccess) {
        return new IR::PrerunValue(
            inst->getLLVMConstant()->getAggregateElement(instType->asCore()->getIndexOf(name.value).value()),
            mem->type);
      } else {
        auto llVal    = ctx->builder.CreateStructGEP(instType->asCore()->getLLVMType(), inst->getLLVM(),
                                                     instType->asCore()->getIndexOf(name.value).value());
        auto memValTy = instType->asCore()->getTypeOfMember(name.value);
        while (memValTy->isReference()) {
          llVal    = ctx->builder.CreateLoad(memValTy->asReference()->getSubType()->getLLVMType(), llVal);
          memValTy = memValTy->asReference()->getSubType();
        }
        return new IR::Value(llVal, IR::ReferenceType::get(isVar, memValTy, ctx), false, IR::Nature::temporary);
      }
    } else if (!(isVariationAccess.has_value() && isVariationAccess.value()) && eTy->hasNormalMemberFn(name.value)) {
      // FIXME - Implement
      ctx->Error("Referencing member function is not supported", fileRange);
    } else if ((isVariationAccess.has_value() && isVariationAccess.value()) && eTy->hasVariationFn(name.value)) {
      // FIXME - Implement
      ctx->Error("Referencing variation function is not supported", fileRange);
    }
  } else if (instType->isPointer() && instType->asPointer()->isMulti()) {
    if (name.value == "length") {
      if (!inst->isImplicitPointer() && !inst->isReference() && !inst->isLLVMConstant() && !isPointerAccess) {
        inst = inst->makeLocal(ctx, None, instance->fileRange);
      }
      if (inst->isLLVMConstant() && !isPointerAccess) {
        return new IR::PrerunValue(inst->getLLVMConstant()->getAggregateElement(1u), IR::CType::getUsize(ctx));
      } else {
        return new IR::Value(
            ctx->builder.CreateLoad(IR::CType::getUsize(ctx)->getLLVMType(),
                                    ctx->builder.CreateStructGEP(instType->getLLVMType(), inst->getLLVM(), 1u)),
            IR::CType::getUsize(ctx), false, IR::Nature::temporary);
      }
    } else {
      ctx->Error("Invalid member name for pointer datatype " + ctx->highlightError(instType->toString()), fileRange);
    }
  } else {
    ctx->Error("Member access for expression of type " + ctx->highlightError(instType->toString()) +
                   " is not supported",
               fileRange);
  }
  return nullptr;
  // NOLINTEND(readability-magic-numbers, clang-analyzer-core.CallAndMessage)
}

Json MemberAccess::toJson() const {
  return Json()
      ._("nodeType", "memberVariable")
      ._("instance", instance->toJson())
      ._("member", name)
      ._("fileRange", fileRange);
}

} // namespace qat::ast
