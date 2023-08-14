#include "./member_access.hpp"
#include "../../IR/types/future.hpp"
#include "../../IR/types/maybe.hpp"
#include "../../IR/types/string_slice.hpp"
#include "entity.hpp"
#include "llvm/IR/Constants.h"
#include "llvm/Support/Casting.h"

namespace qat::ast {

MemberAccess::MemberAccess(Expression* _instance, String _name, FileRange _fileRange)
    : Expression(std::move(_fileRange)), instance(_instance), name(std::move(_name)) {}

IR::Value* MemberAccess::emit(IR::Context* ctx) {
  // NOLINTBEGIN(readability-magic-numbers, clang-analyzer-core.CallAndMessage)
  SHOW("Member variable emitting")
  if (instance->nodeType() == NodeType::entity) {
    ((ast::Entity*)instance)->setCanBeChoice();
  }
  auto* inst     = instance->emit(ctx);
  auto* instType = inst->getType();
  bool  isVar    = inst->isVariable();
  if (instType->isReference()) {
    inst->loadImplicitPointer(ctx->builder);
    isVar    = instType->asReference()->isSubtypeVariable();
    instType = instType->asReference()->getSubType();
  }
  if (instType->isChoice()) {
    auto* chTy = instType->asChoice();
    if (chTy->hasField(name)) {
      auto chInd = chTy->getValueFor(name);
      return new IR::ConstantValue(llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, chTy->getBitwidth()),
                                                          static_cast<u64>(chInd),
                                                          chTy->hasNegativeValues() && (chInd < 0)),
                                   chTy);
    } else {
      ctx->Error("No variant named " + ctx->highlightError(name) + " found in choice type " +
                     ctx->highlightError(chTy->getFullName()),
                 fileRange);
    }
  } else if (instType->isArray()) {
    if (name == "length") {
      return new IR::ConstantValue(
          llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), instType->asArray()->getLength()),
          // NOLINTNEXTLINE(readability-magic-numbers)
          IR::IntegerType::get(64u, ctx->llctx));
    } else {
      ctx->Error("Invalid name for member access " + ctx->highlightError(name) + " for expression with type " +
                     ctx->highlightError(instType->toString()),
                 fileRange);
    }
  } else if (instType->isStringSlice()) {
    if (!inst->isReference() && !inst->isImplicitPointer() && !inst->isLLVMConstant()) {
      inst->makeImplicitPointer(ctx, None);
    }
    if (name == "length") {
      if (inst->isLLVMConstant()) {
        SHOW("String slice is a constant")
        return new IR::ConstantValue(llvm::cast<llvm::ConstantInt>(inst->getLLVMConstant()->getAggregateElement(1u)),
                                     IR::UnsignedType::get(64u, ctx->llctx));
      } else {
        SHOW("String slice is an implicit pointer or a reference")
        return new IR::Value(
            ctx->builder.CreateStructGEP(IR::StringSliceType::get(ctx->llctx)->getLLVMType(), inst->getLLVM(), 1u),
            IR::ReferenceType::get(false, IR::UnsignedType::get(64u, ctx->llctx), ctx), false, IR::Nature::temporary);
      }
    } else if (name == "buffer") {
      if (inst->isLLVMConstant()) {
        return new IR::ConstantValue(inst->getLLVMConstant()->getAggregateElement(0u),
                                     IR::PointerType::get(false, IR::UnsignedType::get(8u, ctx->llctx),
                                                          IR::PointerOwner::OfAnonymous(), false, ctx));
      } else {
        SHOW("String slice is an implicit pointer or a reference")
        return new IR::Value(
            ctx->builder.CreateStructGEP(IR::StringSliceType::get(ctx->llctx)->getLLVMType(), inst->getLLVM(), 0u),
            IR::PointerType::get(false, IR::UnsignedType::get(8u, ctx->llctx), // NOLINT(readability-magic-numbers)
                                 IR::PointerOwner::OfAnonymous(), false, ctx),
            false, IR::Nature::temporary);
      }
    } else {
      ctx->Error("Invalid name for member access: " + ctx->highlightError(name), fileRange);
    }
  } else if (instType->isFuture()) {
    if (name == "isDone") {
      if (inst->isReference() || inst->isImplicitPointer()) {
        return new IR::Value(
            ctx->builder.CreateICmpEQ(
                ctx->builder.CreateLoad(
                    llvm::Type::getInt1Ty(ctx->llctx),
                    ctx->builder.CreateLoad(
                        llvm::Type::getInt1Ty(ctx->llctx)->getPointerTo(),
                        ctx->builder.CreateStructGEP(instType->asFuture()->getLLVMType(), inst->getLLVM(), 2u)),
                    true),
                llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u)),
            IR::UnsignedType::getBool(ctx->llctx), false, IR::Nature::temporary);
      } else {
        ctx->Error("Invalid value for " + ctx->highlightError("future") + " and hence cannot get data", fileRange);
      }
    } else if (name == "isNotDone") {
      if (inst->isImplicitPointer() || inst->isReference()) {
        return new IR::Value(
            ctx->builder.CreateICmpEQ(
                ctx->builder.CreateLoad(
                    llvm::Type::getInt1Ty(ctx->llctx),
                    ctx->builder.CreateLoad(
                        llvm::Type::getInt1Ty(ctx->llctx)->getPointerTo(),
                        ctx->builder.CreateStructGEP(instType->asFuture()->getLLVMType(), inst->getLLVM(), 2u)),
                    true),
                llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 0u)),
            IR::UnsignedType::getBool(ctx->llctx), false, IR::Nature::temporary);
      } else {
        ctx->Error("Invalid value for future and hence cannot get data", fileRange);
      }
    } else {
      ctx->Error("Invalid name " + ctx->highlightError(name) + " for member access of type " +
                     ctx->highlightError(instType->toString()),
                 fileRange);
    }
  } else if (instType->isMaybe()) {
    if (!inst->isImplicitPointer() && !inst->isReference() && !inst->isLLVMConstant()) {
      inst->makeImplicitPointer(ctx, None);
    }
    if (name == "hasValue") {
      if (inst->isLLVMConstant()) {
        if (instType->asMaybe()->hasSizedSubType(ctx)) {
          return new IR::ConstantValue(llvm::cast<llvm::ConstantInt>(inst->getLLVMConstant()->getAggregateElement(0u)),
                                       IR::UnsignedType::getBool(ctx->llctx));
        } else {
          return new IR::ConstantValue(llvm::cast<llvm::ConstantInt>(inst->getLLVMConstant()),
                                       IR::UnsignedType::getBool(ctx->llctx));
        }
      } else {
        if (instType->asMaybe()->hasSizedSubType(ctx)) {
          return new IR::Value(
              ctx->builder.CreateICmpEQ(
                  ctx->builder.CreateLoad(llvm::Type::getInt1Ty(ctx->llctx),
                                          ctx->builder.CreateStructGEP(instType->getLLVMType(), inst->getLLVM(), 0u)),
                  llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u)),
              IR::UnsignedType::getBool(ctx->llctx), false, IR::Nature::temporary);
        } else {
          return new IR::Value(
              ctx->builder.CreateICmpEQ(ctx->builder.CreateLoad(llvm::Type::getInt1Ty(ctx->llctx), inst->getLLVM()),
                                        llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u)),
              IR::UnsignedType::getBool(ctx->llctx), false, IR::Nature::temporary);
        }
      }
    } else if (name == "hasNoValue") {
      if (inst->isLLVMConstant()) {
        if (instType->asMaybe()->hasSizedSubType(ctx)) {
          return new IR::ConstantValue(
              llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx),
                                     llvm::cast<llvm::ConstantInt>(inst->getLLVMConstant()->getAggregateElement(0u))
                                             ->getValue()
                                             .getBoolValue()
                                         ? 0u
                                         : 1u),
              IR::UnsignedType::getBool(ctx->llctx));
        } else {
          return new IR::ConstantValue(
              llvm::ConstantInt::get(
                  llvm::Type::getInt1Ty(ctx->llctx),
                  llvm::cast<llvm::ConstantInt>(inst->getLLVMConstant())->getValue().getBoolValue() ? 0u : 1u),
              IR::UnsignedType::getBool(ctx->llctx));
        }
      } else {
        if (instType->asMaybe()->hasSizedSubType(ctx)) {
          return new IR::Value(
              ctx->builder.CreateICmpEQ(
                  ctx->builder.CreateLoad(llvm::Type::getInt1Ty(ctx->llctx),
                                          ctx->builder.CreateStructGEP(instType->getLLVMType(), inst->getLLVM(), 0u)),
                  llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 0u)),
              IR::UnsignedType::getBool(ctx->llctx), false, IR::Nature::temporary);
        } else {
          return new IR::Value(
              ctx->builder.CreateICmpEQ(ctx->builder.CreateLoad(llvm::Type::getInt1Ty(ctx->llctx), inst->getLLVM()),
                                        llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 0u)),
              IR::UnsignedType::getBool(ctx->llctx), false, IR::Nature::temporary);
        }
      }
    } else {
      ctx->Error("Invalid name " + ctx->highlightError(name) + " for member access of type " +
                     ctx->highlightError(instType->toString()),
                 fileRange);
    }
  } else if (instType->isCoreType()) {
    if (!instType->asCore()->hasMember(name)) {
      ctx->Error("Core type " + ctx->highlightError(instType->asCore()->toString()) + " does not have a member named " +
                     ctx->highlightError(name) + ". Please check the logic",
                 fileRange);
    }
    auto* cTy = instType->asCore();
    auto* mem = cTy->getMemberAt(instType->asCore()->getIndexOf(name).value());
    if (!mem->visibility.isAccessible(ctx->getAccessInfo())) {
      ctx->Error("Member " + ctx->highlightError(name) + " of core type " + ctx->highlightError(cTy->getFullName()) +
                     " is not accessible here",
                 fileRange);
    }
    if (!inst->isImplicitPointer() && !inst->getType()->isReference() && !inst->getType()->isPointer() &&
        !inst->isLLVMConstant()) {
      inst->makeImplicitPointer(ctx, None);
    }
    if (inst->isLLVMConstant()) {
      return new IR::ConstantValue(
          inst->getLLVMConstant()->getAggregateElement(instType->asCore()->getIndexOf(name).value()), mem->type);
    } else {
      return new IR::Value(ctx->builder.CreateStructGEP(instType->asCore()->getLLVMType(), inst->getLLVM(),
                                                        instType->asCore()->getIndexOf(name).value()),
                           IR::ReferenceType::get(isVar, instType->asCore()->getTypeOfMember(name), ctx), false,
                           IR::Nature::temporary);
    }
  } else if (instType->isPointer()) {
    if (name == "length") {
      if (instType->asPointer()->isMulti()) {
        if (!inst->isImplicitPointer() && !inst->isReference() && !inst->isLLVMConstant()) {
          inst->makeImplicitPointer(ctx, None);
        }
        if (inst->isLLVMConstant()) {
          return new IR::ConstantValue(inst->getLLVMConstant()->getAggregateElement(1u),
                                       IR::UnsignedType::get(64u, ctx->llctx));
        } else {
          return new IR::Value(
              ctx->builder.CreateLoad(llvm::Type::getInt64Ty(ctx->llctx),
                                      ctx->builder.CreateStructGEP(instType->getLLVMType(), inst->getLLVM(), 1u)),
              IR::UnsignedType::get(64u, ctx->llctx), false, IR::Nature::temporary);
        }
      } else {
        ctx->Error("The pointer is not a multi pointer and hence the length cannot be determined", fileRange);
      }
    } else {
      ctx->Error("Invalid member name for pointer datatype", fileRange);
    }
  } else {
    ctx->Error("Member access for this type is not supported", fileRange);
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
