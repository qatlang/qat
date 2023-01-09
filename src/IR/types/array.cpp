#include "./array.hpp"
#include "../../memory_tracker.hpp"
#include "../context.hpp"
#include "../control_flow.hpp"
#include "../value.hpp"
#include "./qat_type.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

ArrayType::ArrayType(QatType* _element_type, u64 _length, llvm::LLVMContext& ctx)
    : element_type(_element_type), length(_length) {
  llvmType = llvm::ArrayType::get(element_type->getLLVMType(), length);
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

QatType* ArrayType::getElementType() { return element_type; }

u64 ArrayType::getLength() const { return length; }

TypeKind ArrayType::typeKind() const { return TypeKind::array; }

String ArrayType::toString() const { return element_type->toString() + "[" + std::to_string(length) + "]"; }

Json ArrayType::toJson() const {
  return Json()._("type", "array")._("subtype", element_type->getID())._("length", std::to_string(length));
}

bool ArrayType::isDestructible() const { return element_type->isDestructible(); }

void ArrayType::destroyValue(IR::Context* ctx, Vec<IR::Value*> vals, IR::Function* fun) {
  if (element_type->isDestructible() && !vals.empty()) {
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
    auto* dstrFn = element_type->asExpanded()->getDestructor();
    (void)dstrFn->call(ctx,
                       {ctx->builder.CreateInBoundsGEP(
                           element_type->getLLVMType(), value->getLLVM(),
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