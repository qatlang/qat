#include "./member_access.hpp"
#include "../../IR/types/maybe.hpp"
#include "entity.hpp"
#include "llvm/Analysis/ConstantFolding.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"

namespace qat::ast {

PrerunMemberAccess::PrerunMemberAccess(PrerunExpression* _expr, Identifier _member, FileRange _fileRange)
    : PrerunExpression(std::move(_fileRange)), expr(_expr), memberName(_member) {}

IR::PrerunValue* PrerunMemberAccess::emit(IR::Context* ctx) {
  auto* irExp = expr->emit(ctx);
  if (irExp->getType()->isTyped()) {
    if (memberName.value == "byteSize") {
      auto* subTy = irExp->getType()->asTyped()->getSubType();
      return new IR::PrerunValue(
          llvm::ConstantInt::get(
              llvm::Type::getInt64Ty(ctx->llctx),
              subTy->isTypeSized()
                  ? ctx->getMod()->getLLVMModule()->getDataLayout().getTypeStoreSize(subTy->getLLVMType())
                  : 1u),
          IR::UnsignedType::get(64u, ctx));
    } else if (memberName.value == "bitSize") {
      auto* subTy = irExp->getType()->asTyped()->getSubType();
      return new IR::PrerunValue(
          llvm::ConstantInt::get(
              llvm::Type::getInt64Ty(ctx->llctx),
              subTy->isTypeSized()
                  ? ctx->getMod()->getLLVMModule()->getDataLayout().getTypeStoreSizeInBits(subTy->getLLVMType())
                  : 8u),
          IR::UnsignedType::get(64u, ctx));
    } else if (memberName.value == "name") {
      auto result = irExp->getType()->asTyped()->getSubType()->toString();
      return new IR::PrerunValue(
          llvm::ConstantStruct::get(llvm::cast<llvm::StructType>(IR::StringSliceType::get(ctx)->getLLVMType()),
                                    {ctx->builder.CreateGlobalStringPtr(result, ctx->getGlobalStringName(), 0U,
                                                                        ctx->getMod()->getLLVMModule()),
                                     llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), result.length())}),
          IR::StringSliceType::get(ctx));
    } else if (memberName.value == "isPacked") {
      return new IR::PrerunValue(
          llvm::ConstantInt::get(
              llvm::Type::getInt1Ty(ctx->llctx),
              irExp->getType()->asTyped()->getSubType()->getLLVMType()->isStructTy() &&
                      llvm::cast<llvm::StructType>(irExp->getType()->asTyped()->getSubType()->getLLVMType())->isPacked()
                  ? 1u
                  : 0u),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "isAnyUnsignedIntType") {
      return new IR::PrerunValue(
          llvm::ConstantInt::get(
              llvm::Type::getInt1Ty(ctx->llctx),
              (irExp->getType()->asTyped()->getSubType()->isUnsignedInteger() ||
               (irExp->getType()->asTyped()->getSubType()->isCType() &&
                irExp->getType()->asTyped()->getSubType()->asCType()->getSubType()->isUnsignedInteger()))
                  ? 1u
                  : 0u),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "isAnySignedIntType") {
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx),
                                 (irExp->getType()->asTyped()->getSubType()->isInteger() ||
                                  (irExp->getType()->asTyped()->getSubType()->isCType() &&
                                   irExp->getType()->asTyped()->getSubType()->asCType()->getSubType()->isInteger()))
                                     ? 1u
                                     : 0u),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "isAnyFloatType") {
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx),
                                 (irExp->getType()->asTyped()->getSubType()->isFloat() ||
                                  (irExp->getType()->asTyped()->getSubType()->isCType() &&
                                   irExp->getType()->asTyped()->getSubType()->asCType()->getSubType()->isFloat()))
                                     ? 1u
                                     : 0u),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "isUnsignedIntType") {
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx),
                                 irExp->getType()->asTyped()->getSubType()->isUnsignedInteger() ? 1u : 0u),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "isSignedIntType") {
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx),
                                 irExp->getType()->asTyped()->getSubType()->isInteger() ? 1u : 0u),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "isFloatType") {
      return new IR::PrerunValue(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx),
                                                        irExp->getType()->asTyped()->getSubType()->isFloat() ? 1u : 0u),
                                 IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "isTupleType") {
      return new IR::PrerunValue(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx),
                                                        irExp->getType()->asTyped()->getSubType()->isTuple() ? 1u : 0u),
                                 IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "isStringSliceType") {
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx),
                                 irExp->getType()->asTyped()->getSubType()->isStringSlice() ? 1u : 0u),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "isStructType") {
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx),
                                 irExp->getType()->asTyped()->getSubType()->isCoreType() ? 1u : 0u),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "isUnderlyingStructType") {
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx),
                                 irExp->getType()->asTyped()->getSubType()->getLLVMType()->isStructTy() ? 1u : 0u),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "isMixType") {
      return new IR::PrerunValue(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx),
                                                        irExp->getType()->asTyped()->getSubType()->isMix() ? 1u : 0u),
                                 IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "isArrayType") {
      return new IR::PrerunValue(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx),
                                                        irExp->getType()->asTyped()->getSubType()->isArray() ? 1u : 0u),
                                 IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "isChoiceType") {
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx),
                                 irExp->getType()->asTyped()->getSubType()->isChoice() ? 1u : 0u),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "isMaybeType") {
      return new IR::PrerunValue(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx),
                                                        irExp->getType()->asTyped()->getSubType()->isMaybe() ? 1u : 0u),
                                 IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "isResultType") {
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx),
                                 irExp->getType()->asTyped()->getSubType()->isResult() ? 1u : 0u),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "isVoidType") {
      return new IR::PrerunValue(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx),
                                                        irExp->getType()->asTyped()->getSubType()->isVoid() ? 1u : 0u),
                                 IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "isFutureType") {
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx),
                                 irExp->getType()->asTyped()->getSubType()->isFuture() ? 1u : 0u),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "isPointerType") {
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx),
                                 irExp->getType()->asTyped()->getSubType()->isPointer() ? 1u : 0u),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "isMultiPointerType") {
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx),
                                 (irExp->getType()->asTyped()->getSubType()->isPointer() &&
                                  irExp->getType()->asTyped()->getSubType()->asPointer()->isMulti())
                                     ? 1u
                                     : 0u),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "isTriviallyCopyable") {
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx),
                                 irExp->getType()->asTyped()->getSubType()->isTriviallyCopyable() ? 1u : 0u),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "isTriviallyMovable") {
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx),
                                 irExp->getType()->asTyped()->getSubType()->isTriviallyMovable() ? 1u : 0u),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "isCopyConstructible") {
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx),
                                 irExp->getType()->asTyped()->getSubType()->isCopyConstructible() ? 1u : 0u),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "isCopyAssignable") {
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx),
                                 irExp->getType()->asTyped()->getSubType()->isCopyAssignable() ? 1u : 0u),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "hasCopy") {
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx),
                                 (irExp->getType()->asTyped()->getSubType()->isTriviallyCopyable()) ||
                                         (irExp->getType()->asTyped()->getSubType()->isCopyConstructible() &&
                                          irExp->getType()->asTyped()->getSubType()->isCopyAssignable())
                                     ? 1u
                                     : 0u),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "hasMove") {
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx),
                                 (irExp->getType()->asTyped()->getSubType()->isTriviallyMovable()) ||
                                         (irExp->getType()->asTyped()->getSubType()->isMoveConstructible() &&
                                          irExp->getType()->asTyped()->getSubType()->isMoveAssignable())
                                     ? 1u
                                     : 0u),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "isDefaultConstructible") {
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx),
                                 irExp->getType()->asTyped()->getSubType()->isDefaultConstructible() ? 1u : 0u),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "hasDefaultValue") {
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx),
                                 irExp->getType()->asTyped()->getSubType()->hasDefaultValue() ? 1u : 0u),
          IR::UnsignedType::getBool(ctx));
    } else {
      ctx->Error(ctx->highlightError(memberName.value) + " is not a recognised attribute for the wrapped type",
                 memberName.range);
    }
  } else if (irExp->getType()->isMaybe()) {
    if (memberName.value == "hasValue") {
      return new IR::PrerunValue(irExp->getLLVMConstant()->getAggregateElement(0u), IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "hasNoValue") {
      return new IR::PrerunValue(
          llvm::ConstantFoldConstant(
              llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_EQ,
                                          irExp->getLLVMConstant()->getAggregateElement(0u),
                                          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 0u)),
              ctx->dataLayout.value()),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "get") {
      if (llvm::cast<llvm::ConstantInt>(
              llvm::cast<llvm::ConstantStruct>(irExp->getLLVMConstant())->getAggregateElement(0u))
              ->getValue()
              .getBoolValue()) {
      } else {
        ctx->Error("This maybe expression does not contain any value, so the underlying value cannot be obtained",
                   fileRange);
      }
    } else {
      ctx->Error("Invalid name for member access for expression of maybe type " +
                     ctx->highlightError(irExp->getType()->toString()),
                 memberName.range);
    }
  } else if (irExp->getType()->isArray()) {
    if (memberName.value == "length") {
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), irExp->getType()->asArray()->getLength()),
          IR::UnsignedType::get(64u, ctx));
    } else {
      ctx->Error("Invalid name for member access for expression of array type " +
                     ctx->highlightError(irExp->getType()->toString()),
                 fileRange);
    }
  } else {
    ctx->Error("The expression is of type " + ctx->highlightError(irExp->getType()->toString()) +
                   " and cannot use member access",
               expr->fileRange);
  }
  return nullptr;
}

Json PrerunMemberAccess::toJson() const {
  return Json()
      ._("nodeType", "prerunMemberAccess")
      ._("expression", expr->toJson())
      ._("memberName", memberName)
      ._("fileRange", fileRange);
}

String PrerunMemberAccess::toString() const { return expr->toString() + "'" + memberName.value; }

} // namespace qat::ast