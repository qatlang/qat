#include "./array.hpp"
#include "../../memory_tracker.hpp"
#include "../context.hpp"
#include "../control_flow.hpp"
#include "../value.hpp"
#include "./qat_type.hpp"
#include "reference.hpp"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

ArrayType::ArrayType(QatType* _element_type, u64 _length, llvm::LLVMContext& llctx)
    : elementType(_element_type), length(_length) {
  llvmType    = llvm::ArrayType::get(elementType->getLLVMType(), length);
  linkingName = "qat'array:[" + elementType->getNameForLinking() + "," + std::to_string(length) + "]";
}

ArrayType* ArrayType::get(QatType* elementType, u64 _length, llvm::LLVMContext& llctx) {
  for (auto* typ : allQatTypes) {
    if (typ->isArray()) {
      if (typ->asArray()->getLength() == _length) {
        if (typ->asArray()->getElementType()->isSame(elementType)) {
          return typ->asArray();
        }
      }
    }
  }
  return new ArrayType(elementType, _length, llctx);
}

QatType* ArrayType::getElementType() { return elementType; }

u64 ArrayType::getLength() const { return length; }

TypeKind ArrayType::typeKind() const { return TypeKind::array; }

String ArrayType::toString() const { return elementType->toString() + "[" + std::to_string(length) + "]"; }

bool ArrayType::canBePrerunGeneric() const { return elementType->canBePrerunGeneric(); }

Maybe<String> ArrayType::toPrerunGenericString(IR::PrerunValue* val) const {
  if (canBePrerunGeneric()) {
    String result("[");
    for (usize i = 0; i < length; i++) {
      auto elemStr = elementType->toPrerunGenericString(new IR::PrerunValue(
          llvm::cast<llvm::ConstantDataArray>(val->getLLVMConstant())->getAggregateElement(i), elementType));
      if (elemStr.has_value()) {
        result += elemStr.value();
      } else {
        return None;
      }
      if (i != (length - 1)) {
        result += ", ";
      }
    }
    result += "]";
    return result;
  } else {
    return None;
  }
}

Maybe<bool> ArrayType::equalityOf(IR::PrerunValue* first, IR::PrerunValue* second) const {
  if (canBePrerunGeneric()) {
    if (first->getType()->isSame(second->getType())) {
      auto* array1 = llvm::cast<llvm::ConstantArray>(first->getLLVMConstant());
      auto* array2 = llvm::cast<llvm::ConstantArray>(second->getLLVMConstant());
      for (usize i = 0; i < length; i++) {
        if (!(elementType
                  ->equalityOf(new IR::PrerunValue(array1->getAggregateElement(i), elementType),
                               new IR::PrerunValue(array2->getAggregateElement(i), elementType))
                  .value())) {
          return false;
        }
      }
      return true;
    } else {
      return false;
    }
  } else {
    return None;
  }
}

bool ArrayType::isTypeSized() const { return length > 0; }

bool ArrayType::isCopyConstructible() const { return elementType->isCopyConstructible(); }

bool ArrayType::isMoveConstructible() const { return elementType->isMoveConstructible(); }

bool ArrayType::isCopyAssignable() const { return elementType->isCopyAssignable(); }

bool ArrayType::isMoveAssignable() const { return elementType->isMoveAssignable(); }

bool ArrayType::isDestructible() const { return elementType->isDestructible(); }

void ArrayType::copyConstructValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) {
  if (isCopyConstructible()) {
    auto* Ty64Int = llvm::Type::getInt64Ty(ctx->llctx);
    auto* firstIndexing =
        ctx->builder.CreateInBoundsGEP(elementType->getLLVMType(), first->getLLVM(),
                                       {llvm::ConstantInt::get(Ty64Int, 0u), llvm::ConstantInt::get(Ty64Int, 0u)});
    auto* secondIndexing =
        ctx->builder.CreateInBoundsGEP(elementType->getLLVMType(), second->getLLVM(),
                                       {llvm::ConstantInt::get(Ty64Int, 0u), llvm::ConstantInt::get(Ty64Int, 0u)});
    for (usize i = 0; i < length; i++) {
      if (i != 0) {
        firstIndexing  = ctx->builder.CreateInBoundsGEP(elementType->getLLVMType(), firstIndexing,
                                                        {llvm::ConstantInt::get(Ty64Int, 1u)});
        secondIndexing = ctx->builder.CreateInBoundsGEP(elementType->getLLVMType(), secondIndexing,
                                                        {llvm::ConstantInt::get(Ty64Int, 1u)});
      }
      elementType->copyConstructValue(
          ctx,
          new IR::Value(firstIndexing, IR::ReferenceType::get(true, elementType, ctx), false, IR::Nature::temporary),
          new IR::Value(secondIndexing, IR::ReferenceType::get(true, elementType, ctx), false, IR::Nature::temporary),
          fun);
    }
  } else {
    ctx->Error("Could not copy construct an instance of type " + ctx->highlightError(toString()), None);
  }
}
void ArrayType::copyAssignValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) {
  if (isCopyAssignable()) {
    auto* Ty64Int = llvm::Type::getInt64Ty(ctx->llctx);
    auto* firstIndexing =
        ctx->builder.CreateInBoundsGEP(elementType->getLLVMType(), first->getLLVM(),
                                       {llvm::ConstantInt::get(Ty64Int, 0u), llvm::ConstantInt::get(Ty64Int, 0u)});
    auto* secondIndexing =
        ctx->builder.CreateInBoundsGEP(elementType->getLLVMType(), second->getLLVM(),
                                       {llvm::ConstantInt::get(Ty64Int, 0u), llvm::ConstantInt::get(Ty64Int, 0u)});
    for (usize i = 0; i < length; i++) {
      if (i != 0) {
        firstIndexing  = ctx->builder.CreateInBoundsGEP(elementType->getLLVMType(), firstIndexing,
                                                        {llvm::ConstantInt::get(Ty64Int, 1u)});
        secondIndexing = ctx->builder.CreateInBoundsGEP(elementType->getLLVMType(), secondIndexing,
                                                        {llvm::ConstantInt::get(Ty64Int, 1u)});
      }
      elementType->copyAssignValue(
          ctx,
          new IR::Value(firstIndexing, IR::ReferenceType::get(true, elementType, ctx), false, IR::Nature::temporary),
          new IR::Value(secondIndexing, IR::ReferenceType::get(true, elementType, ctx), false, IR::Nature::temporary),
          fun);
    }
  } else {
    ctx->Error("Could not copy assign an instance of type " + ctx->highlightError(toString()), None);
  }
}

void ArrayType::moveConstructValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) {
  if (isMoveAssignable()) {
    auto* Ty64Int = llvm::Type::getInt64Ty(ctx->llctx);
    auto* firstIndexing =
        ctx->builder.CreateInBoundsGEP(elementType->getLLVMType(), first->getLLVM(),
                                       {llvm::ConstantInt::get(Ty64Int, 0u), llvm::ConstantInt::get(Ty64Int, 0u)});
    auto* secondIndexing =
        ctx->builder.CreateInBoundsGEP(elementType->getLLVMType(), second->getLLVM(),
                                       {llvm::ConstantInt::get(Ty64Int, 0u), llvm::ConstantInt::get(Ty64Int, 0u)});
    for (usize i = 0; i < length; i++) {
      if (i != 0) {
        firstIndexing  = ctx->builder.CreateInBoundsGEP(elementType->getLLVMType(), firstIndexing,
                                                        {llvm::ConstantInt::get(Ty64Int, 1u)});
        secondIndexing = ctx->builder.CreateInBoundsGEP(elementType->getLLVMType(), secondIndexing,
                                                        {llvm::ConstantInt::get(Ty64Int, 1u)});
      }
      elementType->moveConstructValue(
          ctx,
          new IR::Value(firstIndexing, IR::ReferenceType::get(true, elementType, ctx), false, IR::Nature::temporary),
          new IR::Value(secondIndexing, IR::ReferenceType::get(true, elementType, ctx), false, IR::Nature::temporary),
          fun);
    }
  } else {
    ctx->Error("Could not move construct an instance of type " + ctx->highlightError(toString()), None);
  }
}

void ArrayType::moveAssignValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) {
  if (isMoveAssignable()) {
    auto* Ty64Int = llvm::Type::getInt64Ty(ctx->llctx);
    auto* firstIndexing =
        ctx->builder.CreateInBoundsGEP(elementType->getLLVMType(), first->getLLVM(),
                                       {llvm::ConstantInt::get(Ty64Int, 0u), llvm::ConstantInt::get(Ty64Int, 0u)});
    auto* secondIndexing =
        ctx->builder.CreateInBoundsGEP(elementType->getLLVMType(), second->getLLVM(),
                                       {llvm::ConstantInt::get(Ty64Int, 0u), llvm::ConstantInt::get(Ty64Int, 0u)});
    for (usize i = 0; i < length; i++) {
      if (i != 0) {
        firstIndexing  = ctx->builder.CreateInBoundsGEP(elementType->getLLVMType(), firstIndexing,
                                                        {llvm::ConstantInt::get(Ty64Int, 1u)});
        secondIndexing = ctx->builder.CreateInBoundsGEP(elementType->getLLVMType(), secondIndexing,
                                                        {llvm::ConstantInt::get(Ty64Int, 1u)});
      }
      elementType->moveAssignValue(
          ctx,
          new IR::Value(firstIndexing, IR::ReferenceType::get(true, elementType, ctx), false, IR::Nature::temporary),
          new IR::Value(secondIndexing, IR::ReferenceType::get(true, elementType, ctx), false, IR::Nature::temporary),
          fun);
    }
  } else {
    ctx->Error("Could not move assign an instance of type " + ctx->highlightError(toString()), None);
  }
}

void ArrayType::destroyValue(IR::Context* ctx, IR::Value* instance, IR::Function* fun) {
  if (elementType->isDestructible()) {
    auto* Ty64Int = llvm::Type::getInt64Ty(ctx->llctx);
    auto* instanceIndexing =
        ctx->builder.CreateInBoundsGEP(elementType->getLLVMType(), instance->getLLVM(),
                                       {llvm::ConstantInt::get(Ty64Int, 0u), llvm::ConstantInt::get(Ty64Int, 0u)});
    for (usize i = 0; i < length; i++) {
      if (i != 0) {
        instanceIndexing = ctx->builder.CreateInBoundsGEP(elementType->getLLVMType(), instanceIndexing,
                                                          {llvm::ConstantInt::get(Ty64Int, 1u)});
      }
      elementType->destroyValue(
          ctx,
          new IR::Value(instanceIndexing, IR::ReferenceType::get(true, elementType, ctx), false, IR::Nature::temporary),
          fun);
    }
  } else {
    ctx->Error("Could not destroy instance of type " + ctx->highlightError(toString()), None);
  }
}

} // namespace qat::IR