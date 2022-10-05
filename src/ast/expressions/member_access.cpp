#include "./member_access.hpp"
#include "../../IR/types/string_slice.hpp"
#include "entity.hpp"
#include "llvm/IR/Constants.h"
#include "llvm/Support/Casting.h"

namespace qat::ast {

MemberAccess::MemberAccess(Expression* _instance, bool _isPointerAccess, String _name, utils::FileRange _fileRange)
    : Expression(std::move(_fileRange)), instance(_instance), name(std::move(_name)), isPointer(_isPointerAccess) {}

IR::Value* MemberAccess::emit(IR::Context* ctx) {
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
  if (isPointer) {
    if (instType->isPointer()) {
      inst->loadImplicitPointer(ctx->builder);
      isVar    = instType->asPointer()->isSubtypeVariable();
      instType = instType->asPointer()->getSubType();
    } else {
      ctx->Error("The expression type has to be a pointer to use >- to access members", instance->fileRange);
    }
  } else {
    if (instType->isPointer()) {
      ctx->Error("The expression is of pointer type. Please use >- to access members", instance->fileRange);
    }
  }
  if (instType->isChoice()) {
    auto* chTy = instType->asChoice();
    if (chTy->hasField(name)) {
      auto chInd = (u64)chTy->getValueFor(name);
      return new IR::Value(llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, chTy->getBitwidth()), chInd), chTy,
                           false, IR::Nature::pure);
    } else {
      ctx->Error("No variant named " + ctx->highlightError(name) + " found in choice type " +
                     ctx->highlightError(chTy->getFullName()),
                 fileRange);
    }
  } else if (instType->isArray()) {
    if (name == "length") {
      return new IR::Value(llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), instType->asArray()->getLength()),
                           // NOLINTNEXTLINE(readability-magic-numbers)
                           IR::IntegerType::get(64u, ctx->llctx), false, IR::Nature::pure);
    } else {
      ctx->Error("Invalid name for member access " + ctx->highlightError(name), fileRange);
    }
  } else if (instType->isStringSlice()) {
    if (name == "length") {
      if (inst->isImplicitPointer() || inst->isReference()) {
        SHOW("String slice is an implicit pointer or a reference")
        return new IR::Value(
            ctx->builder.CreateStructGEP(IR::StringSliceType::get(ctx->llctx)->getLLVMType(), inst->getLLVM(), 1u),
            IR::ReferenceType::get(false, IR::UnsignedType::get(64u, ctx->llctx), ctx->llctx), false,
            IR::Nature::temporary);
      } else if (llvm::isa<llvm::Constant>(inst->getLLVM())) {
        SHOW("String slice is a constant")
        return new IR::Value(
            llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx),
                                   llvm::dyn_cast<llvm::ConstantDataArray>(inst->getLLVM())->getAsString().size()),
            IR::ReferenceType::get(false, IR::UnsignedType::get(64u, ctx->llctx), ctx->llctx), false,
            IR::Nature::temporary);
      } else {
        ctx->Error("Invalid value for String Slice and hence cannot get length", fileRange);
      }
    } else if (name == "data") {
      if (inst->isImplicitPointer() || inst->isReference()) {
        SHOW("String slice is an implicit pointer or a reference")
        return new IR::Value(
            ctx->builder.CreateStructGEP(IR::StringSliceType::get(ctx->llctx)->getLLVMType(), inst->getLLVM(), 0u),
            IR::ReferenceType::get(false, IR::UnsignedType::get(8u, ctx->llctx), ctx->llctx), false,
            IR::Nature::temporary);
      } else if (llvm::isa<llvm::Constant>(inst->getLLVM())) {
        // FIXME - Implement this
        return nullptr;
      } else {
        ctx->Error("Invalid value for String Slice and hence cannot get data", fileRange);
      }
    } else {
      ctx->Error("Invalid name for member access: " + ctx->highlightError(name), fileRange);
    }
  } else if (instType->isCoreType()) {
    if (!instType->asCore()->hasMember(name)) {
      ctx->Error("Core type " + ctx->highlightError(instType->asCore()->toString()) + " does not have a member named " +
                     ctx->highlightError(name) + ". Please check the logic",
                 fileRange);
    }
    auto* cTy = instType->asCore();
    auto* mem = cTy->getMemberAt(instType->asCore()->getIndexOf(name).value());
    if (!mem->visibility.isAccessible(ctx->getReqInfo())) {
      ctx->Error("Member " + ctx->highlightError(name) + " of core type " + ctx->highlightError(cTy->getFullName()) +
                     " is not accessible here",
                 fileRange);
    }
    if (!inst->isImplicitPointer() && !inst->getType()->isReference() && !inst->getType()->isPointer()) {
      inst = inst->createAlloca(ctx->builder);
    }
    return new IR::Value(ctx->builder.CreateStructGEP(instType->asCore()->getLLVMType(), inst->getLLVM(),
                                                      instType->asCore()->getIndexOf(name).value()),
                         IR::ReferenceType::get(isVar, instType->asCore()->getTypeOfMember(name), ctx->llctx), false,
                         IR::Nature::temporary);
  } else {
    ctx->Error("Member access for this type is not supported", fileRange);
  }
  return nullptr;
}

Json MemberAccess::toJson() const {
  return Json()
      ._("nodeType", "memberVariable")
      ._("instance", instance->toJson())
      ._("isPointerAccess", isPointer)
      ._("member", name)
      ._("fileRange", fileRange);
}

} // namespace qat::ast