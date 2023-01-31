#include "./array.hpp"
#include "../../memory_tracker.hpp"
#include "../context.hpp"
#include "../control_flow.hpp"
#include "../value.hpp"
#include "./qat_type.hpp"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

ArrayType::ArrayType(QatType* _element_type, u64 _length, llvm::LLVMContext& ctx)
    : elementType(_element_type), length(_length) {
  llvmType = llvm::ArrayType::get(elementType->getLLVMType(), length);
}

ArrayType* ArrayType::get(QatType* elementType, u64 _length, llvm::LLVMContext& ctx) {
  for (auto* typ : types) {
    if (typ->isArray()) {
      if (typ->asArray()->getLength() == _length) {
        if (typ->asArray()->getElementType()->isSame(elementType)) {
          return typ->asArray();
        }
      }
    }
  }
  return new ArrayType(elementType, _length, ctx);
}

QatType* ArrayType::getElementType() { return elementType; }

u64 ArrayType::getLength() const { return length; }

TypeKind ArrayType::typeKind() const { return TypeKind::array; }

String ArrayType::toString() const { return elementType->toString() + "[" + std::to_string(length) + "]"; }

Json ArrayType::toJson() const {
  return Json()._("type", "array")._("subtype", elementType->getID())._("length", std::to_string(length));
}

bool ArrayType::canBeConstGeneric() const { return elementType->canBeConstGeneric(); }

Maybe<String> ArrayType::toConstGenericString(IR::ConstantValue* val) const {
  if (canBeConstGeneric()) {
    String result("[");
    for (usize i = 0; i < length; i++) {
      auto elemStr = elementType->toConstGenericString(new IR::ConstantValue(
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

Maybe<bool> ArrayType::equalityOf(IR::ConstantValue* first, IR::ConstantValue* second) const {
  if (canBeConstGeneric()) {
    if (first->getType()->isSame(second->getType())) {
      auto* array1 = llvm::cast<llvm::ConstantArray>(first->getLLVMConstant());
      auto* array2 = llvm::cast<llvm::ConstantArray>(second->getLLVMConstant());
      for (usize i = 0; i < length; i++) {
        if (!(elementType
                  ->equalityOf(new IR::ConstantValue(array1->getAggregateElement(i), elementType),
                               new IR::ConstantValue(array2->getAggregateElement(i), elementType))
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

bool ArrayType::isDestructible() const { return elementType->isDestructible(); }

void ArrayType::destroyValue(IR::Context* ctx, Vec<IR::Value*> vals, IR::Function* fun) {
  if (elementType->isDestructible() && !vals.empty()) {
    auto* value = vals.front();
    if (value->isReference()) {
      value->loadImplicitPointer(ctx->builder);
    }
    auto* currBlock = fun->getBlock();
    auto* condBlock = new IR::Block(fun, currBlock);
    auto* trueBlock = new IR::Block(fun, currBlock);
    auto* restBlock = new IR::Block(fun, currBlock->getParent());
    restBlock->linkPrevBlock(currBlock);
    auto* count   = fun->getFunctionCommonIndex();
    auto* Ty64Int = llvm::Type::getInt64Ty(ctx->llctx);
    ctx->builder.CreateStore(llvm::ConstantInt::get(Ty64Int, 0u), count->getLLVM());
    (void)IR::addBranch(ctx->builder, condBlock->getBB());
    condBlock->setActive(ctx->builder);
    ctx->builder.CreateCondBr(ctx->builder.CreateICmpULT(ctx->builder.CreateLoad(Ty64Int, count->getLLVM()),
                                                         llvm::ConstantInt::get(Ty64Int, getLength())),
                              trueBlock->getBB(), restBlock->getBB());
    trueBlock->setActive(ctx->builder);
    auto* dstrFn = elementType->asExpanded()->getDestructor();
    (void)dstrFn->call(ctx,
                       {ctx->builder.CreateInBoundsGEP(
                           elementType->getLLVMType(), value->getLLVM(),
                           {llvm::ConstantInt::get(Ty64Int, 0u), ctx->builder.CreateLoad(Ty64Int, count->getLLVM())})},
                       ctx->getMod());
    ctx->builder.CreateStore(
        ctx->builder.CreateAdd(ctx->builder.CreateLoad(Ty64Int, count->getLLVM()), llvm::ConstantInt::get(Ty64Int, 1u)),
        count->getLLVM());
    (void)IR::addBranch(ctx->builder, condBlock->getBB());
    restBlock->setActive(ctx->builder);
  }
}

} // namespace qat::IR